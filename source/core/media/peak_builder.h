#pragma once

#include "core/media/peak_store.h"

#include <juce_core/juce_core.h>

#include <deque>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

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
