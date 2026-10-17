#pragma once

#include "engine/plugins/plugin_processor.h"

#include <memory>
#include <vector>

class PluginChain
{
public:
	struct Slot
	{
		std::shared_ptr<PluginProcessor> processor;
		bool bypass = false;

		bool operator==(const Slot&) const = default;
	};

	explicit PluginChain(std::vector<Slot> slots);

	const std::vector<Slot>& slots() const { return chain; }
	int total_latency() const;
	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead);

private:
	int longest_tail() const;

	std::vector<Slot> chain;
	int remaining_tail = 0;
	bool ringing = false;
};
