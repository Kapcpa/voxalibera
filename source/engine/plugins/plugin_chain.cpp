#include "engine/plugins/plugin_chain.h"

namespace
{
	constexpr float SILENCE = 1.0e-5f;
}

PluginChain::PluginChain(std::vector<Slot> slots) : chain(std::move(slots)), remaining_tail(longest_tail())
{
}

int PluginChain::total_latency() const
{
	int total = 0;

	for (const auto& slot : chain)
		if (!slot.bypass)
			total += slot.processor->latency_samples();

	return total;
}

int PluginChain::longest_tail() const
{
	int longest = 0;

	for (const auto& slot : chain)
		if (!slot.bypass)
			longest = std::max(longest, slot.processor->tail_samples() + slot.processor->latency_samples());

	return longest;
}

void PluginChain::process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead)
{
	const auto silent = buffer.getMagnitude(0, sample_count) < SILENCE;

	if (silent && remaining_tail <= 0)
		return;

	remaining_tail = silent ? remaining_tail - sample_count : longest_tail();

	for (const auto& slot : chain)
		if (!slot.bypass)
			slot.processor->process(buffer, sample_count, playhead);
}
