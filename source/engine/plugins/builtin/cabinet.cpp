#include "engine/plugins/builtin/cabinet.h"

#include "engine/dsp/convolution_queue.h"

#include <cmath>

namespace
{
	constexpr double SMOOTHING_SECONDS = 0.05;
	constexpr int TONE_BLOCK = 32;
	constexpr float PERCENT = 100.0f;
	constexpr float LONGEST_DELAY_MICROSECONDS = 1000.0f;
	constexpr double MICROSECONDS = 1.0e6;
	constexpr float BUTTERWORTH_Q = 0.7071f;
	constexpr float RESONANCE_HERTZ = 100.0f;
	constexpr float RESONANCE_Q = 1.4f;
	constexpr float PRESENCE_HERTZ = 3500.0f;
	constexpr size_t A = 0;
	constexpr size_t B = 1;
	const juce::StringArray POLARITY_CHOICES{"Normal", "Inverted"};
}

Cabinet::Microphone::Microphone() : convolution(convolution_queue())
{
}

Cabinet::Cabinet() : microphone_a(add_file({.key = "microphone-a", .name = "Mic A", .empty_text = "Generic cabinet"})), microphone_b(add_file({.key = "microphone-b", .name = "Mic B", .empty_text = "None"})), blend(add_parameter({.key = "blend", .name = "A / B", .maximum = PERCENT, .default_value = 50.0f, .unit = "%", .decimals = 0})), b_delay(add_parameter({.key = "b-delay", .name = "B Delay", .maximum = LONGEST_DELAY_MICROSECONDS, .default_value = 0.0f, .unit = "us", .decimals = 0, .centre = 200.0f})), b_polarity(add_parameter({.key = "b-polarity", .name = "B Polarity", .choices = POLARITY_CHOICES})), low_cut(add_parameter({.key = "low-cut", .name = "Low Cut", .minimum = 20.0f, .maximum = 300.0f, .default_value = 20.0f, .unit = "Hz", .decimals = 0, .centre = 80.0f})), high_cut(add_parameter({.key = "high-cut", .name = "High Cut", .minimum = 2000.0f, .maximum = 20000.0f, .default_value = 20000.0f, .unit = "Hz", .decimals = 0, .centre = 7000.0f})), resonance(add_parameter({.key = "resonance", .name = "Resonance", .maximum = 9.0f, .default_value = 0.0f, .unit = "dB"})), presence(add_parameter({.key = "presence", .name = "Presence", .maximum = 9.0f, .default_value = 0.0f, .unit = "dB"})), output(add_parameter({.key = "output", .name = "Output", .minimum = -24.0f, .maximum = 24.0f, .default_value = 0.0f, .unit = "dB"}))
{
	send(A, generic_cabinet_response());
}

Cabinet::~Cabinet()
{
	Delivery delivery;

	while (deliveries.pop(delivery))
		delete delivery.response;

	free_delivered();
}

std::unique_ptr<ImpulseResponse> Cabinet::response_for(size_t microphone)
{
	const auto& file = microphone == A ? microphone_a : microphone_b;

	if (file.path().isNotEmpty())
		if (auto response = read_impulse_response(resolve(file)))
			return response;

	return microphone == A ? generic_cabinet_response() : no_impulse_response();
}

void Cabinet::file_changed(BuiltinFile& file)
{
	const auto microphone = &file == &microphone_a ? A : B;
	send(microphone, response_for(microphone));
}

void Cabinet::send(size_t microphone, std::unique_ptr<ImpulseResponse> response)
{
	free_delivered();
	microphones[microphone].seconds.store(response->seconds());

	if (!prepared) {
		install(microphone, *response);
		return;
	}

	if (deliveries.push({microphone, response.get()}))
		response.release();
}

void Cabinet::install(size_t microphone, ImpulseResponse& response)
{
	auto& target = microphones[microphone];
	target.loaded = !response.is_empty();

	if (target.loaded)
		target.convolution.loadImpulseResponse(std::move(response.samples), response.sample_rate, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
}

void Cabinet::install_deliveries()
{
	Delivery delivery;

	while (deliveries.pop(delivery)) {
		install(delivery.microphone, *delivery.response);
		delivered.push(delivery.response);
	}
}

void Cabinet::free_delivered()
{
	ImpulseResponse* response = nullptr;

	while (delivered.pop(response))
		delete response;
}

int Cabinet::tail_samples() const
{
	return (int) std::ceil(std::max(microphones[A].seconds.load(), microphones[B].seconds.load()) * sample_rate);
}

void Cabinet::prepare(const PluginProcessContext& context)
{
	sample_rate = context.sample_rate;
	const juce::dsp::ProcessSpec spec{sample_rate, (juce::uint32) context.maximum_block_size, (juce::uint32) CHANNELS};

	for (auto& microphone : microphones)
		microphone.convolution.prepare(spec);

	microphone_b_buffer.setSize(CHANNELS, context.maximum_block_size);
	alignment.prepare(spec);
	alignment.setMaximumDelayInSamples((int) std::ceil(LONGEST_DELAY_MICROSECONDS * sample_rate / MICROSECONDS) + 4);

	for (auto* smoothed : {&blend_amount, &delay_samples, &low_cut_hertz, &high_cut_hertz, &resonance_db, &presence_db})
		smoothed->reset(sample_rate, SMOOTHING_SECONDS);

	output_gain.reset(sample_rate, SMOOTHING_SECONDS);
	blend_amount.setCurrentAndTargetValue(microphones[B].loaded ? blend.value() / PERCENT : 0.0f);
	delay_samples.setCurrentAndTargetValue((float) (b_delay.value() * sample_rate / MICROSECONDS));
	low_cut_hertz.setCurrentAndTargetValue(low_cut.value());
	high_cut_hertz.setCurrentAndTargetValue(high_cut.value());
	resonance_db.setCurrentAndTargetValue(resonance.value());
	presence_db.setCurrentAndTargetValue(presence.value());
	output_gain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(output.value()));

	for (auto& channel : tone)
		channel = {};

	set_tone();
	prepared = true;
}

void Cabinet::process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead&)
{
	install_deliveries();
	mix_microphones(buffer, sample_count);
	shape_tone(buffer, sample_count);
}

void Cabinet::mix_microphones(juce::AudioBuffer<float>& buffer, int sample_count)
{
	for (int channel = 0; channel < CHANNELS; ++channel)
		microphone_b_buffer.copyFrom(channel, 0, buffer, channel, 0, sample_count);

	juce::dsp::AudioBlock<float> block_a(buffer.getArrayOfWritePointers(), (size_t) CHANNELS, (size_t) sample_count);
	juce::dsp::AudioBlock<float> block_b(microphone_b_buffer.getArrayOfWritePointers(), (size_t) CHANNELS, (size_t) sample_count);
	microphones[A].convolution.process(juce::dsp::ProcessContextReplacing<float>(block_a));
	microphones[B].convolution.process(juce::dsp::ProcessContextReplacing<float>(block_b));

	blend_amount.setTargetValue(microphones[B].loaded ? blend.value() / PERCENT : 0.0f);
	delay_samples.setTargetValue((float) (b_delay.value() * sample_rate / MICROSECONDS));
	const auto polarity = b_polarity.value() >= 1.0f ? -1.0f : 1.0f;

	for (int index = 0; index < sample_count; ++index) {
		const auto amount = blend_amount.getNextValue();
		const auto delay = delay_samples.getNextValue();

		for (int channel = 0; channel < CHANNELS; ++channel) {
			alignment.pushSample(channel, microphone_b_buffer.getSample(channel, index));
			const auto aligned_b = polarity * alignment.popSample(channel, delay);
			auto* sample = buffer.getWritePointer(channel, index);
			*sample = (1.0f - amount) * *sample + amount * aligned_b;
		}
	}
}

void Cabinet::set_tone()
{
	const auto low = BiquadCoefficients::highpass(sample_rate, low_cut_hertz.getCurrentValue(), BUTTERWORTH_Q);
	const auto high = BiquadCoefficients::lowpass(sample_rate, high_cut_hertz.getCurrentValue(), BUTTERWORTH_Q);
	const auto bump = BiquadCoefficients::peak(sample_rate, RESONANCE_HERTZ, RESONANCE_Q, resonance_db.getCurrentValue());
	const auto shelf = BiquadCoefficients::high_shelf(sample_rate, PRESENCE_HERTZ, presence_db.getCurrentValue());

	for (auto& channel : tone) {
		channel.low_cut.set(low);
		channel.high_cut.set(high);
		channel.resonance.set(bump);
		channel.presence.set(shelf);
	}
}

void Cabinet::shape_tone(juce::AudioBuffer<float>& buffer, int sample_count)
{
	low_cut_hertz.setTargetValue(low_cut.value());
	high_cut_hertz.setTargetValue(high_cut.value());
	resonance_db.setTargetValue(resonance.value());
	presence_db.setTargetValue(presence.value());
	output_gain.setTargetValue(juce::Decibels::decibelsToGain(output.value()));

	for (int start = 0; start < sample_count; start += TONE_BLOCK) {
		const auto count = std::min(TONE_BLOCK, sample_count - start);
		const auto moving = low_cut_hertz.isSmoothing() || high_cut_hertz.isSmoothing() || resonance_db.isSmoothing() || presence_db.isSmoothing();

		for (auto* smoothed : {&low_cut_hertz, &high_cut_hertz, &resonance_db, &presence_db})
			smoothed->skip(count);

		if (moving)
			set_tone();

		for (int channel = 0; channel < CHANNELS; ++channel) {
			auto& filters = tone[(size_t) channel];
			auto* samples = buffer.getWritePointer(channel, start);

			for (int index = 0; index < count; ++index)
				samples[index] = filters.presence.process(filters.resonance.process(filters.high_cut.process(filters.low_cut.process(samples[index]))));
		}
	}

	output_gain.applyGain(buffer, sample_count);
}
