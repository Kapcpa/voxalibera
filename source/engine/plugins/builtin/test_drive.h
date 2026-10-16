#pragma once

#include "engine/plugins/builtin/builtin_processor.h"

class TestDrive : public BuiltinProcessor
{
public:
	TestDrive();

	void prepare(const PluginProcessContext& context) override;
	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead) override;

private:
	BuiltinParameter& drive;
	BuiltinParameter& mix;
	BuiltinParameter& output;
	juce::SmoothedValue<float> drive_gain;
	juce::SmoothedValue<float> wet;
	juce::SmoothedValue<float> output_gain;
};
