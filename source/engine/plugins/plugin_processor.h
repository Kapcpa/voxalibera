#pragma once

#include "engine/musical_time.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <cstdint>
#include <memory>

namespace juce
{
	class Component;
}

struct PluginProcessContext
{
	double sample_rate = 0.0;
	int maximum_block_size = 0;
};

struct PluginPlayhead
{
	bool playing = false;
	bool recording = false;
	std::int64_t sample = 0;
	Tempo tempo;
};

class PluginProcessor
{
public:
	virtual ~PluginProcessor() = default;

	virtual void prepare(const PluginProcessContext& context) = 0;
	virtual void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead) = 0;

	virtual juce::MemoryBlock get_state() const { return {}; }
	virtual void set_state(const juce::MemoryBlock&) {}
	virtual int latency_samples() const { return 0; }
	virtual int tail_samples() const { return 0; }
	virtual std::unique_ptr<juce::Component> create_editor();

	int changes() const { return change_count.load(std::memory_order_relaxed); }

protected:
	void mark_changed() { change_count.fetch_add(1, std::memory_order_relaxed); }

private:
	std::atomic<int> change_count{0};
};
