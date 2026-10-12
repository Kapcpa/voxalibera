#pragma once

#include "engine/plugins/plugin_processor.h"
#include <cmath>

class TestDriveProcessor : public PluginProcessor
{
public:
	void prepare(const PluginProcessContext&) override {}
	void reset() override {}

	void process(juce::AudioBuffer<float>& buffer, int num_samples) override
	{
		for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
			float* data = buffer.getWritePointer(ch);
			for (int i = 0; i < num_samples; ++i) {
				data[i] = std::tanh(data[i] * 2.5f);
			}
		}
	}
};