#include "engine/plugins/hosted_plugin.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr double MIN_TAIL_SECONDS = 2.0;
	constexpr double MAX_TAIL_SECONDS = 30.0;
	constexpr int MIDI_EVENTS_RESERVED = 256;
	constexpr double SECONDS_PER_MINUTE = 60.0;
	constexpr double QUARTERS_PER_WHOLE_NOTE = 4.0;

	struct Layout
	{
		int inputs;
		int outputs;
	};

	constexpr Layout PREFERRED_LAYOUTS[] = {{2, 2}, {1, 2}, {1, 1}};
}

HostedPlugin::HostedPlugin(std::unique_ptr<juce::AudioPluginInstance> loaded) : instance(std::move(loaded))
{
	midi.ensureSize(MIDI_EVENTS_RESERVED);
	choose_layout();
	instance->setPlayHead(this);
	instance->addListener(this);
}

HostedPlugin::~HostedPlugin()
{
	instance->removeListener(this);
	instance->setPlayHead(nullptr);
	instance->releaseResources();
}

void HostedPlugin::choose_layout()
{
	instance->disableNonMainBuses();

	for (const auto preferred : PREFERRED_LAYOUTS) {
		auto layout = instance->getBusesLayout();

		if (!layout.inputBuses.isEmpty())
			layout.inputBuses.getReference(0) = juce::AudioChannelSet::canonicalChannelSet(preferred.inputs);

		if (!layout.outputBuses.isEmpty())
			layout.outputBuses.getReference(0) = juce::AudioChannelSet::canonicalChannelSet(preferred.outputs);

		if (instance->setBusesLayout(layout))
			break;
	}

	input_channels = instance->getTotalNumInputChannels();
	output_channels = instance->getTotalNumOutputChannels();
}

void HostedPlugin::prepare(const PluginProcessContext& context)
{
	sample_rate = context.sample_rate;
	instance->setRateAndBufferSizeDetails(context.sample_rate, context.maximum_block_size);
	instance->prepareToPlay(context.sample_rate, context.maximum_block_size);

	const auto tail_seconds = instance->getTailLengthSeconds();
	tail = (int) (std::clamp(std::isfinite(tail_seconds) ? tail_seconds : MAX_TAIL_SECONDS, MIN_TAIL_SECONDS, MAX_TAIL_SECONDS) * sample_rate);
}

void HostedPlugin::process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead)
{
	playhead_now = playhead;
	juce::AudioBuffer<float> channels(buffer.getArrayOfWritePointers(), std::max(input_channels, output_channels), sample_count);
	midi.clear();
	instance->processBlock(channels, midi);

	if (output_channels == 1)
		buffer.copyFrom(1, 0, buffer, 0, 0, sample_count);
}

juce::MemoryBlock HostedPlugin::get_state() const
{
	juce::MemoryBlock state;
	instance->getStateInformation(state);
	return state;
}

void HostedPlugin::set_state(const juce::MemoryBlock& state)
{
	if (!state.isEmpty())
		instance->setStateInformation(state.getData(), (int) state.getSize());
}

int HostedPlugin::latency_samples() const
{
	return instance->getLatencySamples();
}

std::unique_ptr<juce::Component> HostedPlugin::create_editor()
{
	if (instance->hasEditor())
		return std::unique_ptr<juce::Component>(instance->createEditorAndMakeActive());

	return std::make_unique<juce::GenericAudioProcessorEditor>(*instance);
}

juce::Optional<juce::AudioPlayHead::PositionInfo> HostedPlugin::getPosition() const
{
	if (sample_rate <= 0.0)
		return {};

	const auto& signature = playhead_now.tempo.signature;
	const auto quarters_per_minute = playhead_now.tempo.quarter_notes_per_minute;
	const auto seconds = (double) playhead_now.sample / sample_rate;
	const auto quarters = seconds * quarters_per_minute / SECONDS_PER_MINUTE;
	const auto quarters_per_bar = signature.beats_per_bar * QUARTERS_PER_WHOLE_NOTE / signature.beat_unit;
	const auto bars = std::floor(quarters / quarters_per_bar);

	PositionInfo position;
	position.setIsPlaying(playhead_now.playing);
	position.setIsRecording(playhead_now.recording);
	position.setTimeInSamples(playhead_now.sample);
	position.setTimeInSeconds(seconds);
	position.setBpm(quarters_per_minute);
	position.setTimeSignature(juce::AudioPlayHead::TimeSignature{signature.beats_per_bar, signature.beat_unit});
	position.setPpqPosition(quarters);
	position.setPpqPositionOfLastBarStart(bars * quarters_per_bar);
	position.setBarCount((juce::int64) bars);
	return position;
}

void HostedPlugin::audioProcessorParameterChanged(juce::AudioProcessor*, int, float)
{
	mark_changed();
}

void HostedPlugin::audioProcessorChanged(juce::AudioProcessor*, const ChangeDetails& details)
{
	if (details.parameterInfoChanged || details.programChanged || details.nonParameterStateChanged)
		mark_changed();
}
