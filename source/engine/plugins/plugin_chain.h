#pragma once

#include "engine/plugins/plugin_processor.h"
#include <atomic>
#include <memory>
#include <vector>

class PluginChain
{
public:
	struct Slot
	{
		std::shared_ptr<PluginProcessor> processor;
		std::atomic<bool> bypass{false};

		Slot(std::shared_ptr<PluginProcessor> proc, bool b = false) : processor(std::move(proc)), bypass(b) {}

		Slot(Slot&& other) noexcept : processor(std::move(other.processor)), bypass(other.bypass.load(std::memory_order_relaxed)) {}

		Slot& operator=(Slot&& other) noexcept
		{
			if (this != &other) {
				processor = std::move(other.processor);
				bypass.store(other.bypass.load(std::memory_order_relaxed), std::memory_order_relaxed);
			}
			return *this;
		}

		Slot(const Slot&) = delete;
		Slot& operator=(const Slot&) = delete;
	};

	PluginChain() = default;

	void prepare(const PluginProcessContext& context);
	void reset();
	void process(juce::AudioBuffer<float>& buffer, int num_samples);

	void add_processor(std::shared_ptr<PluginProcessor> processor);
	void clear();

	void set_bypassed(size_t index, bool bypassed);
	bool is_bypassed(size_t index) const noexcept;
	bool empty() const noexcept { return slots.empty(); }

	int total_latency() const noexcept;

private:
	std::vector<Slot> slots;
	int tail_counter_samples{0};
};