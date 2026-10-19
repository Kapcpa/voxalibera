#pragma once

#include "engine/dsp/filters.h"
#include "engine/dsp/impulse_response.h"
#include "engine/lock_free.h"
#include "engine/plugins/builtin/builtin_processor.h"

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

class Cabinet : public BuiltinProcessor
{
public:
	Cabinet();
	~Cabinet() override;

	void prepare(const PluginProcessContext& context) override;
	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead) override;
	int tail_samples() const override;

private:
	static constexpr int CHANNELS = 2;
	static constexpr size_t MICROPHONES = 2;
	static constexpr size_t PENDING_RESPONSES = 8;

	struct Microphone
	{
		juce::dsp::Convolution convolution;
		bool loaded = false;
		std::atomic<float> seconds{0.0f};

		Microphone();
	};

	struct Delivery
	{
		size_t microphone = 0;
		ImpulseResponse* response = nullptr;
	};

	struct Tone
	{
		Biquad low_cut;
		Biquad high_cut;
		Biquad resonance;
		Biquad presence;
	};

	void file_changed(BuiltinFile& file) override;
	std::unique_ptr<ImpulseResponse> response_for(size_t microphone);
	void send(size_t microphone, std::unique_ptr<ImpulseResponse> response);
	void install(size_t microphone, ImpulseResponse& response);
	void install_deliveries();
	void free_delivered();
	void mix_microphones(juce::AudioBuffer<float>& buffer, int sample_count);
	void shape_tone(juce::AudioBuffer<float>& buffer, int sample_count);
	void set_tone();

	BuiltinFile& microphone_a;
	BuiltinFile& microphone_b;
	BuiltinParameter& blend;
	BuiltinParameter& b_delay;
	BuiltinParameter& b_polarity;
	BuiltinParameter& low_cut;
	BuiltinParameter& high_cut;
	BuiltinParameter& resonance;
	BuiltinParameter& presence;
	BuiltinParameter& output;

	std::array<Microphone, MICROPHONES> microphones;
	LockFreeQueue<Delivery, PENDING_RESPONSES> deliveries;
	LockFreeQueue<ImpulseResponse*, PENDING_RESPONSES> delivered;
	juce::AudioBuffer<float> microphone_b_buffer;
	juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> alignment;
	std::array<Tone, CHANNELS> tone;
	juce::SmoothedValue<float> blend_amount;
	juce::SmoothedValue<float> delay_samples;
	juce::SmoothedValue<float> low_cut_hertz;
	juce::SmoothedValue<float> high_cut_hertz;
	juce::SmoothedValue<float> resonance_db;
	juce::SmoothedValue<float> presence_db;
	juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> output_gain;
	double sample_rate = 0.0;
	bool prepared = false;
};
