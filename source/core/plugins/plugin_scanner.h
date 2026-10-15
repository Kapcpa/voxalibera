#pragma once

#include "core/plugins/plugin_library.h"
#include "core/settings.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <vector>

constexpr auto SCAN_PLUGINS_FLAG = "--scan-plugins";

struct ScanProgress
{
	bool running = false;
	int done = 0;
	int total = 0;
	juce::String current;
};

struct ScanFailure
{
	juce::String format;
	juce::String identifier;
	juce::String reason;
	bool blacklisted = false;
};

class PluginScanner : private juce::Thread
{
public:
	PluginScanner(PluginLibrary& library, const Settings& settings);
	~PluginScanner() override;

	void start(bool rescan_everything);
	void cancel();
	bool update();
	ScanProgress progress() const;

	std::vector<ScanFailure> failures;

private:
	struct Job
	{
		juce::String format;
		juce::StringArray identifiers;
	};

	struct Found
	{
		juce::String identifier;
		juce::Array<juce::PluginDescription> descriptions;
	};

	struct ScanOutput
	{
		std::vector<Found> found;
		juce::String unfinished;
	};

	enum class ChildOutcome
	{
		finished,
		stopped_responding,
		cancelled,
	};

	void run() override;
	void scan(const Job& job);
	ChildOutcome watch(juce::ChildProcess& child, const juce::File& output_file, int done_before);
	void deliver(std::vector<Found> arrived);
	void fail(const juce::String& format, const juce::StringArray& identifiers, const juce::String& reason, bool blacklist);
	void set_current(const juce::String& identifier);
	juce::StringArray identifiers_to_scan(juce::AudioPluginFormat& format, bool rescan_everything);
	void forget_removed_plugins(juce::AudioPluginFormat& format);
	static ScanOutput read_scan_output(const juce::File& output_file);

	PluginLibrary& library;
	const Settings& settings;
	std::vector<Job> jobs;
	int total = 0;
	std::atomic<int> done{0};
	juce::CriticalSection lock;
	juce::String current;
	std::vector<Found> found;
	std::vector<ScanFailure> failed;
};

int run_plugin_scan(const juce::StringArray& arguments);
