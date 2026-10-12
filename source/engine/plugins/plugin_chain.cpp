#include "engine/plugins/plugin_chain.h"

void PluginChain::prepare(const PluginProcessContext& context)
{
	tail_counter_samples = 0;
	for (auto& slot : slots)
		if (slot.processor)
			slot.processor->prepare(context);
}

void PluginChain::reset()
{
	tail_counter_samples = 0;
	for (auto& slot : slots)
		if (slot.processor)
			slot.processor->reset();
}

void PluginChain::add_processor(std::shared_ptr<PluginProcessor> processor)
{
	slots.emplace_back(Slot{std::move(processor), false});
}

void PluginChain::clear()
{
	slots.clear();
	tail_counter_samples = 0;
}

void PluginChain::set_bypassed(size_t index, bool bypassed)
{
	if (index < slots.size())
		slots[index].bypass.store(bypassed, std::memory_order_relaxed);
}

bool PluginChain::is_bypassed(size_t index) const noexcept
{
	return (index < slots.size()) ? slots[index].bypass.load(std::memory_order_relaxed) : true;
}

int PluginChain::total_latency() const noexcept
{
	int total = 0;
	for (const auto& slot : slots)
		if (slot.processor && !slot.bypass.load(std::memory_order_relaxed))
			total += slot.processor->latency_samples();
	return total;
}

void PluginChain::process(juce::AudioBuffer<float>& buffer, int num_samples)
{
	if (slots.empty())
		return;

	const float magnitude = buffer.getMagnitude(0, num_samples);
	const bool is_silent = (magnitude < 1e-5f);

	if (is_silent) {
		if (tail_counter_samples <= 0)
			return;
		tail_counter_samples -= num_samples;
	}

	for (auto& slot : slots) {
		if (!slot.processor || slot.bypass.load(std::memory_order_relaxed))
			continue;

		slot.processor->process(buffer, num_samples);

		const int tail = slot.processor->tail_samples();
		if (!is_silent && tail > tail_counter_samples)
			tail_counter_samples = tail;
	}
}