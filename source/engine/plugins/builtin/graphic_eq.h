#pragma once

#include "engine/dsp/filters.h"
#include "engine/plugins/builtin/builtin_processor.h"

class GraphicEq : public BuiltinProcessor
{
public:
	GraphicEq();

	void prepare(const PluginProcessContext& context) override;
	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead) override;

private:
	void set_band(size_t band, float gain_db);

	static constexpr double SMOOTHING_SECONDS = 0.02;

	static constexpr size_t BANDS = 7;
	static constexpr size_t CHANNELS = 2;

	static constexpr std::array<float, BANDS> BAND_FREQUENCIES{100, 200, 400, 800, 1600, 3200, 6400};

	std::array<BuiltinParameter*, BANDS> band_levels;
	std::array<std::array<Svf, CHANNELS>, BANDS> filter_state;
	std::array<juce::SmoothedValue<float>, BANDS> smoothed_gains;

	BuiltinParameter* level_gain;
	juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smoothed_level_gain;

	double sample_rate = 0.0;
};
