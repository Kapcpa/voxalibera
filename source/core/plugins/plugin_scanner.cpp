#include "core/plugins/plugin_scanner.h"

#include "core/log.h"
#include "platform/system.h"

namespace
{
	constexpr auto LIST_FILE = "scan-list.txt";
	constexpr auto OUTPUT_FILE = "scan-output.txt";
	constexpr auto SCANNING_LINE = "scanning";
	constexpr auto FOUND_LINE = "found";
	constexpr auto PLUGINS_ELEMENT = "plugins";
	constexpr auto SEPARATOR = "\t";
	constexpr int POLL_MILLISECONDS = 100;
	constexpr double STOPPED_RESPONDING_SECONDS = 30.0;
	constexpr int STOP_TIMEOUT_MILLISECONDS = 5000;
	constexpr int SCAN_ARGUMENT_COUNT = 4;
	constexpr int EXIT_OK = 0;
	constexpr int EXIT_BAD_ARGUMENTS = 2;
	constexpr int EXIT_UNKNOWN_FORMAT = 3;
	constexpr int EXIT_UNWRITABLE_OUTPUT = 4;

	double now_in_seconds()
	{
		return juce::Time::getMillisecondCounterHiRes() / 1000.0;
	}

	juce::File scan_file(const char* name)
	{
		return app_cache_directory().getChildFile(name);
	}

	juce::AudioPluginFormat* find_format(const juce::AudioPluginFormatManager& formats, const juce::String& name)
	{
		for (auto* format : formats.getFormats())
			if (format->getName() == name)
				return format;

		return nullptr;
	}

	juce::String describe_exit(const juce::ChildProcess& child)
	{
		return "exit code " + juce::String(child.getExitCode());
	}
}

PluginScanner::PluginScanner(PluginLibrary& library, const Settings& settings) : juce::Thread("Plugin scanner"), library(library), settings(settings)
{
}

PluginScanner::~PluginScanner()
{
	stopThread(STOP_TIMEOUT_MILLISECONDS);
}

void PluginScanner::start(bool rescan_everything)
{
	if (isThreadRunning())
		return;

	update();
	failures.clear();
	jobs.clear();
	total = 0;
	done = 0;

	for (auto* format : library.formats.getFormats()) {
		if (!format->canScanForPlugins())
			continue;

		forget_removed_plugins(*format);
		const auto identifiers = identifiers_to_scan(*format, rescan_everything);

		if (identifiers.isEmpty())
			continue;

		total += identifiers.size();
		jobs.push_back({format->getName(), identifiers});
	}

	library.save();
	log_event("Plugin scan started: " + juce::String(total) + " to check" + (rescan_everything ? ", rescanning everything" : ""));

	if (!jobs.empty())
		startThread();
}

void PluginScanner::cancel()
{
	signalThreadShouldExit();
}

ScanProgress PluginScanner::progress() const
{
	const juce::ScopedLock scoped(lock);
	return {isThreadRunning(), done.load(), total, current};
}

bool PluginScanner::update()
{
	std::vector<Found> arrived;
	std::vector<ScanFailure> newly_failed;

	{
		const juce::ScopedLock scoped(lock);
		std::swap(arrived, found);
		std::swap(newly_failed, failed);
	}

	for (const auto& failure : newly_failed) {
		if (failure.blacklisted)
			library.known.addToBlacklist(failure.identifier);

		log_event("Plugin scan: " + failure.identifier + " (" + failure.format + ") " + failure.reason);
		failures.push_back(failure);
	}

	for (const auto& plugin : arrived)
		for (const auto& description : plugin.descriptions)
			library.known.addType(description);

	if (!arrived.empty() || !newly_failed.empty())
		library.save();

	return !arrived.empty();
}

juce::StringArray PluginScanner::identifiers_to_scan(juce::AudioPluginFormat& format, bool rescan_everything)
{
	auto folders = format.getDefaultLocationsToSearch();

	for (const auto& folder : settings.plugin_folders)
		folders.addIfNotAlreadyThere(juce::File(folder));

	juce::StringArray identifiers;

	for (const auto& identifier : format.searchPathsForPlugins(folders, true, false)) {
		const auto blacklisted = library.known.getBlacklistedFiles().contains(identifier);

		if (rescan_everything) {
			library.known.removeFromBlacklist(identifier);
			identifiers.add(identifier);
		} else if (!blacklisted && !library.known.isListingUpToDate(identifier, format)) {
			identifiers.add(identifier);
		}
	}

	return identifiers;
}

void PluginScanner::forget_removed_plugins(juce::AudioPluginFormat& format)
{
	for (const auto& description : library.known.getTypes())
		if (description.pluginFormatName == format.getName() && !format.doesPluginStillExist(description))
			library.known.removeType(description);
}

void PluginScanner::run()
{
	for (const auto& job : jobs) {
		if (threadShouldExit())
			break;

		scan(job);
	}

	set_current({});
	scan_file(LIST_FILE).deleteFile();
	scan_file(OUTPUT_FILE).deleteFile();
}

void PluginScanner::scan(const Job& job)
{
	const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName();
	const auto list_file = scan_file(LIST_FILE);
	const auto output_file = scan_file(OUTPUT_FILE);
	auto remaining = job.identifiers;

	while (!remaining.isEmpty() && !threadShouldExit()) {
		list_file.getParentDirectory().createDirectory();
		list_file.replaceWithText(remaining.joinIntoString("\n"));
		output_file.deleteFile();

		juce::ChildProcess child;

		if (!child.start(juce::StringArray{executable, SCAN_PLUGINS_FLAG, job.format, list_file.getFullPathName(), output_file.getFullPathName()}, 0)) {
			fail(job.format, remaining, "was not scanned: the scanner process could not start", false);
			return;
		}

		const auto done_before = done.load();
		const auto outcome = watch(child, output_file, done_before);
		auto output = read_scan_output(output_file);
		const auto progressed = !output.found.empty() || output.unfinished.isNotEmpty();

		for (const auto& plugin : output.found)
			remaining.removeString(plugin.identifier);

		done = done_before + (int) output.found.size();
		deliver(std::move(output.found));

		if (outcome == ChildOutcome::cancelled)
			return;

		if (output.unfinished.isNotEmpty()) {
			const auto reason = outcome == ChildOutcome::stopped_responding ? "stopped responding while being scanned" : "crashed while being scanned (" + describe_exit(child) + ")";
			fail(job.format, {output.unfinished}, reason, true);
			remaining.removeString(output.unfinished);
			continue;
		}

		if (!progressed && !remaining.isEmpty()) {
			fail(job.format, remaining, "was not scanned: the scanner process stopped first (" + describe_exit(child) + ")", false);
			return;
		}
	}
}

PluginScanner::ChildOutcome PluginScanner::watch(juce::ChildProcess& child, const juce::File& output_file, int done_before)
{
	juce::int64 last_size = -1;
	auto last_progress = now_in_seconds();

	while (!child.waitForProcessToFinish(POLL_MILLISECONDS)) {
		if (threadShouldExit()) {
			child.kill();
			return ChildOutcome::cancelled;
		}

		const auto size = output_file.getSize();

		if (size != last_size) {
			last_size = size;
			last_progress = now_in_seconds();
			const auto output = read_scan_output(output_file);
			done = done_before + (int) output.found.size();
			set_current(output.unfinished);
			continue;
		}

		if (now_in_seconds() - last_progress > STOPPED_RESPONDING_SECONDS) {
			child.kill();
			return ChildOutcome::stopped_responding;
		}
	}

	return ChildOutcome::finished;
}

void PluginScanner::deliver(std::vector<Found> arrived)
{
	const juce::ScopedLock scoped(lock);

	for (auto& plugin : arrived)
		found.push_back(std::move(plugin));
}

void PluginScanner::fail(const juce::String& format, const juce::StringArray& identifiers, const juce::String& reason, bool blacklist)
{
	const juce::ScopedLock scoped(lock);

	for (const auto& identifier : identifiers)
		failed.push_back({format, identifier, reason, blacklist});

	done += identifiers.size();
}

void PluginScanner::set_current(const juce::String& identifier)
{
	const juce::ScopedLock scoped(lock);
	current = identifier;
}

PluginScanner::ScanOutput PluginScanner::read_scan_output(const juce::File& output_file)
{
	ScanOutput output;

	for (const auto& line : juce::StringArray::fromLines(output_file.loadFileAsString())) {
		const auto kind = line.upToFirstOccurrenceOf(SEPARATOR, false, false);
		const auto rest = line.fromFirstOccurrenceOf(SEPARATOR, false, false);

		if (kind == SCANNING_LINE) {
			output.unfinished = rest;
			continue;
		}

		if (kind != FOUND_LINE)
			continue;

		Found plugin{rest.upToFirstOccurrenceOf(SEPARATOR, false, false), {}};

		if (const auto xml = juce::parseXML(rest.fromFirstOccurrenceOf(SEPARATOR, false, false)))
			for (const auto* element : xml->getChildIterator())
				if (juce::PluginDescription description; description.loadFromXml(*element))
					plugin.descriptions.add(description);

		if (output.unfinished == plugin.identifier)
			output.unfinished = {};

		output.found.push_back(std::move(plugin));
	}

	return output;
}

int run_plugin_scan(const juce::StringArray& arguments)
{
	const auto flag = arguments.indexOf(SCAN_PLUGINS_FLAG);

	if (flag < 0 || arguments.size() < flag + SCAN_ARGUMENT_COUNT)
		return EXIT_BAD_ARGUMENTS;

	juce::AudioPluginFormatManager formats;
	juce::addDefaultFormatsToManager(formats);
	auto* format = find_format(formats, arguments[flag + 1]);

	if (format == nullptr)
		return EXIT_UNKNOWN_FORMAT;

	auto identifiers = juce::StringArray::fromLines(juce::File(arguments[flag + 2]).loadFileAsString());
	identifiers.removeEmptyStrings();
	const juce::File output_file{arguments[flag + 3]};
	juce::FileOutputStream output{output_file};

	if (!output.openedOk())
		return EXIT_UNWRITABLE_OUTPUT;

	for (const auto& identifier : identifiers) {
		output << SCANNING_LINE << SEPARATOR << identifier << "\n";
		output.flush();

		juce::OwnedArray<juce::PluginDescription> descriptions;
		format->findAllTypesForFile(descriptions, identifier);

		juce::XmlElement plugins(PLUGINS_ELEMENT);

		for (const auto* description : descriptions)
			plugins.addChildElement(description->createXml().release());

		output << FOUND_LINE << SEPARATOR << identifier << SEPARATOR << plugins.toString(juce::XmlElement::TextFormat().singleLine().withoutHeader()) << "\n";
		output.flush();
	}

	return EXIT_OK;
}
