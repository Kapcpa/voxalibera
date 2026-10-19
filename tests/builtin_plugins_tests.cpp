#include "engine/plugins/builtin/builtin_plugins.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
	constexpr double SAMPLE_RATE = 44100.0;
	constexpr int BLOCK = 512;
	constexpr int BLOCKS = 64;
	constexpr float FREQUENCY = 220.0f;
	constexpr float AMPLITUDE = 0.5f;

	bool is_finite(const juce::AudioBuffer<float>& buffer)
	{
		for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
			for (int index = 0; index < buffer.getNumSamples(); ++index)
				if (!std::isfinite(buffer.getSample(channel, index)))
					return false;

		return true;
	}
}

TEST_CASE("every built-in effect makes finite sound and keeps its state")
{
	for (const auto& plugin : BUILTIN_PLUGINS) {
		INFO(plugin.identifier);
		auto processor = plugin.create();
		processor->prepare({SAMPLE_RATE, BLOCK});

		juce::AudioBuffer<float> buffer(2, BLOCK);
		float loudest = 0.0f;

		for (int block = 0; block < BLOCKS; ++block) {
			for (int index = 0; index < BLOCK; ++index) {
				const auto phase = juce::MathConstants<float>::twoPi * FREQUENCY * (float) (block * BLOCK + index) / (float) SAMPLE_RATE;
				buffer.setSample(0, index, AMPLITUDE * std::sin(phase));
				buffer.setSample(1, index, AMPLITUDE * std::sin(phase));
			}

			processor->process(buffer, BLOCK, {});
			REQUIRE(is_finite(buffer));
			loudest = std::max(loudest, buffer.getMagnitude(0, BLOCK));
		}

		CHECK(loudest > 0.01f);

		const auto state = processor->get_state();
		auto copy = plugin.create();
		copy->set_state(state);
		CHECK(copy->get_state() == state);
	}
}
