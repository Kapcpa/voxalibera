#include "engine/dsp/impulse_response.h"

#include "engine/dsp/filters.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <cmath>
#include <vector>

namespace
{
	constexpr double LONGEST_SECONDS = 0.25;
	constexpr double FADE_SECONDS = 0.01;
	constexpr float ONSET_DECIBELS = -50.0f;
	constexpr float NORMALISED_DECIBELS = -9.0f;
	constexpr int ONSET_PREROLL = 4;
	constexpr double GENERIC_RATE = 48000.0;
	constexpr double GENERIC_SECONDS = 0.1;

	struct GenericBand
	{
		float frequency;
		float q;
		float gain_db;
	};

	constexpr float GENERIC_LOW_CUT = 75.0f;
	constexpr float GENERIC_HIGH_CUT = 4800.0f;
	constexpr GenericBand GENERIC_RESONANCE{110.0f, 1.4f, 5.0f};
	constexpr GenericBand GENERIC_BOX_DIP{450.0f, 1.0f, -3.0f};
	constexpr GenericBand GENERIC_PRESENCE{2400.0f, 1.0f, 4.0f};

	int onset(const float* samples, int length)
	{
		const auto peak = juce::FloatVectorOperations::findMaximum(samples, length);
		const auto lowest = std::max(peak, -juce::FloatVectorOperations::findMinimum(samples, length));
		const auto threshold = lowest * juce::Decibels::decibelsToGain(ONSET_DECIBELS);

		for (int index = 0; index < length; ++index)
			if (std::abs(samples[index]) >= threshold)
				return std::max(0, index - ONSET_PREROLL);

		return 0;
	}

	void fade_out(float* samples, int length, int fade_length)
	{
		const auto start = std::max(0, length - fade_length);

		for (int index = start; index < length; ++index) {
			const auto progress = (float) (index - start) / (float) std::max(1, fade_length);
			samples[index] *= 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * progress));
		}
	}

	void normalise_energy(float* samples, int length)
	{
		double energy = 0.0;

		for (int index = 0; index < length; ++index)
			energy += (double) samples[index] * samples[index];

		if (energy > 0.0)
			juce::FloatVectorOperations::multiply(samples, juce::Decibels::decibelsToGain(NORMALISED_DECIBELS) / (float) std::sqrt(energy), length);
	}

	std::unique_ptr<ImpulseResponse> shaped(const float* source, int source_length, double sample_rate)
	{
		const auto start = onset(source, source_length);
		const auto length = std::min(source_length - start, (int) (LONGEST_SECONDS * sample_rate));

		auto response = std::make_unique<ImpulseResponse>();
		response->sample_rate = sample_rate;
		response->samples.setSize(1, length);
		response->samples.copyFrom(0, 0, source + start, length);

		auto* samples = response->samples.getWritePointer(0);
		fade_out(samples, length, (int) (FADE_SECONDS * sample_rate));
		normalise_energy(samples, length);
		return response;
	}
}

float ImpulseResponse::seconds() const
{
	return sample_rate > 0.0 ? (float) (samples.getNumSamples() / sample_rate) : 0.0f;
}

std::unique_ptr<ImpulseResponse> read_impulse_response(const juce::File& file)
{
	juce::AudioFormatManager formats;
	formats.registerBasicFormats();
	const std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));

	if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
		return nullptr;

	const auto length = (int) std::min<juce::int64>(reader->lengthInSamples, (juce::int64) (LONGEST_SECONDS * reader->sampleRate * 2.0));
	juce::AudioBuffer<float> source(1, length);
	reader->read(&source, 0, length, 0, true, false);
	return shaped(source.getReadPointer(0), length, reader->sampleRate);
}

std::unique_ptr<ImpulseResponse> generic_cabinet_response()
{
	const auto length = (int) (GENERIC_SECONDS * GENERIC_RATE);
	std::vector<float> samples((size_t) length, 0.0f);
	samples.front() = 1.0f;

	std::array<Svf, 6> stages;
	stages[0].set(SvfCoefficients::make(FilterShape::highpass, GENERIC_RATE, GENERIC_LOW_CUT, butterworth_q(2, 0)));
	stages[1].set(SvfCoefficients::make(FilterShape::bell, GENERIC_RATE, GENERIC_RESONANCE.frequency, GENERIC_RESONANCE.q, GENERIC_RESONANCE.gain_db));
	stages[2].set(SvfCoefficients::make(FilterShape::bell, GENERIC_RATE, GENERIC_BOX_DIP.frequency, GENERIC_BOX_DIP.q, GENERIC_BOX_DIP.gain_db));
	stages[3].set(SvfCoefficients::make(FilterShape::bell, GENERIC_RATE, GENERIC_PRESENCE.frequency, GENERIC_PRESENCE.q, GENERIC_PRESENCE.gain_db));
	stages[4].set(SvfCoefficients::make(FilterShape::lowpass, GENERIC_RATE, GENERIC_HIGH_CUT, butterworth_q(2, 0)));
	stages[5].set(SvfCoefficients::make(FilterShape::lowpass, GENERIC_RATE, GENERIC_HIGH_CUT, butterworth_q(2, 0)));

	for (auto& sample : samples)
		for (auto& stage : stages)
			sample = stage.process(sample);

	return shaped(samples.data(), length, GENERIC_RATE);
}

std::unique_ptr<ImpulseResponse> no_impulse_response()
{
	return std::make_unique<ImpulseResponse>();
}
