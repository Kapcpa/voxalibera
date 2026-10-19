#pragma once

#include "engine/dsp/envelope_follower.h"
#include "engine/plugins/builtin/builtin_processor.h"

class NoiseGate : public BuiltinProcessor
{
public:
	NoiseGate();

	void prepare(const PluginProcessContext& context) override;
	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead) override;

private:
	float smoothing_for(float seconds) const;

	BuiltinParameter& threshold;
	BuiltinParameter& hold;
	BuiltinParameter& release;
	BuiltinParameter& range;
	EnvelopeFollower detector;
	double sample_rate = 0.0;
	float gain = 0.0f;
	int hold_left = 0;
	bool open = false;
};
