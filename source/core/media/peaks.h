#pragma once

#include <juce_core/juce_core.h>

#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

struct Peaks
{
	int samples_per_peak = 0;
	std::vector<std::vector<float>> channels;
};

class PeakStore
{
public:
	const Peaks* find(const juce::String& file) const;
	void store(const juce::String& file, Peaks peaks);
	void clear();

private:
	std::map<juce::String, Peaks> peaks_by_file;
};

juce::File peak_cache_file(const juce::File& project_folder, const juce::String& file);

class PeakBuilder : private juce::Thread
{
public:
	PeakBuilder();
	~PeakBuilder() override;

	void request(const juce::File& project_folder, const juce::String& file);
	void cancel_all();
	void deliver_to(PeakStore& store);

private:
	struct Job
	{
		juce::File audio;
		juce::File cache;
		juce::String file;
		int generation = 0;
	};

	void run() override;
	std::optional<Job> next_job();
	std::optional<Peaks> build(const Job& job);

	std::mutex mutex;
	std::deque<Job> jobs;
	std::vector<std::pair<juce::String, Peaks>> finished;
	int generation = 0;
};
