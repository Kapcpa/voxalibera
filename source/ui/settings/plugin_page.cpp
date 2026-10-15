#include "ui/settings/plugin_page.h"

#include "ui/settings/settings_parts.h"

namespace
{
	constexpr int BUTTON_SPACING = 10;
	constexpr int BUTTON_HEIGHT = 26;
	constexpr int BUTTON_GAP = 6;
	constexpr int DEFAULT_FOLDERS_LINES = 4;

	juce::String plural(int count, const juce::String& noun)
	{
		return juce::String(count) + " " + noun + (count == 1 ? "" : "s");
	}

	juce::String short_name(const juce::String& identifier)
	{
		return juce::File::isAbsolutePath(identifier) ? juce::File(identifier).getFileName() : identifier;
	}
}

PluginPage::PluginPage(AppContext& context) : context(context)
{
	scan_button.onClick = [this] { start_scan(false); };
	rescan_button.onClick = [this] { start_scan(true); };
	add_plugin_folder_button.onClick = [this] { add_plugin_folder(); };

	default_folders_label.setText("Searched by default: \n" + context.library.default_folders().joinIntoString("\n"), juce::dontSendNotification);
	default_folders_label.setJustificationType(juce::Justification::topLeft);
	addAndMakeVisible(default_folders_label);

	addAndMakeVisible(title);

	addAndMakeVisible(scan_button);
	addAndMakeVisible(rescan_button);
	addAndMakeVisible(add_plugin_folder_button);

	addAndMakeVisible(scan_status);
	addAndMakeVisible(folder_label);

	rebuild_folder_rows();
}

void PluginPage::update()
{
	const auto running = context.scanner.progress().running;

	scan_button.setEnabled(!running);
	rescan_button.setEnabled(!running);
	scan_status.setText(describe_scan(), juce::dontSendNotification);
}

void PluginPage::start_scan(bool rescan_everything)
{
	plugins_before_scan = context.library.known.getNumTypes();
	context.scanner.start(rescan_everything);
}

juce::String PluginPage::describe_scan() const
{
	const auto progress = context.scanner.progress();

	if (progress.running) {
		auto text = "Scanning " + juce::String(juce::jmin(progress.done + 1, progress.total)) + "/" + juce::String(progress.total);
		return progress.current.isEmpty() ? text : text + ": " + short_name(progress.current);
	}

	const auto plugin_count = context.library.known.getNumTypes();
	auto text = plural(plugin_count, "external plugin");

	if (plugins_before_scan >= 0) {
		const auto added = plugin_count - plugins_before_scan;
		text = (added > 0 ? plural(added, "new plugin") : juce::String("No new plugins")) + ", " + text + " in total";
	}

	if (const auto failed = (int) context.scanner.failures.size(); failed > 0)
		text += ", " + juce::String(failed) + " could not be loaded";

	return text;
}

void PluginPage::rebuild_folder_rows()
{
	folder_rows.clear();

	for (int i = 0; i < context.settings.plugin_folders.size(); ++i) {
		auto row = std::make_unique<FolderRow>();

		row->path_label.setText(context.settings.plugin_folders[i], juce::dontSendNotification);
		row->path_label.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
		addAndMakeVisible(row->path_label);

		row->remove_button.onClick = [this, index = i] {
			context.settings.plugin_folders.remove(index);
			rebuild_folder_rows();
			resized();
		};
		addAndMakeVisible(row->remove_button);

		folder_rows.push_back(std::move(row));
	}
}

void PluginPage::add_plugin_folder()
{
	file_chooser = std::make_unique<juce::FileChooser>("Select the folder containing plugins you want to add...", juce::File::getSpecialLocation(juce::File::userHomeDirectory));

	constexpr auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

	file_chooser->launchAsync(flags, [this](const juce::FileChooser& chooser) {
		const auto result = chooser.getResult();

		if (result.isDirectory()) {
			const auto path = result.getFullPathName();

			if (!context.settings.plugin_folders.contains(path)) {
				context.settings.plugin_folders.add(path);
				rebuild_folder_rows();
				resized();
			}
		}
	});
}

void PluginPage::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds();

	title.setBounds(area.removeFromTop(SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);

	auto scan_row = area.removeFromTop(BUTTON_HEIGHT);

	rescan_button.setBounds(scan_row.removeFromRight(BUTTON_WIDTH));
	scan_row.removeFromRight(BUTTON_GAP);
	scan_button.setBounds(scan_row.removeFromRight(BUTTON_WIDTH + BUTTON_SPACING));
	scan_row.removeFromRight(BUTTON_GAP);
	scan_status.setBounds(scan_row);

	area.removeFromTop(ROW_GAP);

	folder_label.setBounds(area.removeFromTop(SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);

	int default_folders = context.library.default_folders().size();
	default_folders_label.setBounds(area.removeFromTop(default_folders * SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);

	for (auto& row : folder_rows) {
		auto row_area = area.removeFromTop(BUTTON_HEIGHT);
		row->remove_button.setBounds(row_area.removeFromRight(0.5 * BUTTON_WIDTH));
		row_area.removeFromRight(BUTTON_GAP);
		row->path_label.setBounds(row_area);
		area.removeFromTop(ROW_GAP);
	}

	add_plugin_folder_button.setBounds(area.removeFromTop(BUTTON_HEIGHT).removeFromLeft(2 * BUTTON_WIDTH));
}
