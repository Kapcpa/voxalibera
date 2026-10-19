#pragma once

#include "engine/dsp/filters.h"
#include "engine/dsp/oversampler.h"
#include "engine/plugins/builtin/builtin_processor.h"

#include <array>

class DrivePedal : public BuiltinProcessor
{
public:
	void prepare(const PluginProcessContext& context) final;
	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead) final;
	int latency_samples() const final { return latency; }

protected:
	static constexpr int CHANNELS = Oversampler::CHANNELS;
	static constexpr double SMOOTHING_SECONDS = 0.02;

	explicit DrivePedal(int oversampling_power);

	BuiltinParameter& add_level(const juce::String& name);
	float output_gain(float level_db, float trim_db) const;

	virtual void prepare_circuit(double oversampled_rate) = 0;
	virtual void process_circuit(float* const* channels, int sample_count) = 0;

private:
	Oversampler oversampler;
	std::array<OnePole, CHANNELS> dc_blockers;
	int latency = 0;
};
