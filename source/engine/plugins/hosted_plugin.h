#pragma once

#include "engine/plugins/plugin_processor.h"

#include <juce_audio_processors/juce_audio_processors.h>

class HostedPlugin : public PluginProcessor, private juce::AudioProcessorListener, private juce::AudioPlayHead
{
public:
	explicit HostedPlugin(std::unique_ptr<juce::AudioPluginInstance> loaded);
	~HostedPlugin() override;

	bool has_usable_layout() const { return std::max(input_channels, output_channels) <= MAX_CHANNELS && output_channels > 0; }

	void prepare(const PluginProcessContext& context) override;
	void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead& playhead) override;
	juce::MemoryBlock get_state() const override;
	void set_state(const juce::MemoryBlock& state) override;
	int latency_samples() const override;
	int tail_samples() const override { return tail; }
	std::unique_ptr<juce::Component> create_editor() override;

private:
	static constexpr int MAX_CHANNELS = 2;

	void choose_layout();
	juce::Optional<PositionInfo> getPosition() const override;
	void audioProcessorParameterChanged(juce::AudioProcessor* processor, int parameter_index, float value) override;
	void audioProcessorChanged(juce::AudioProcessor* processor, const ChangeDetails& details) override;

	std::unique_ptr<juce::AudioPluginInstance> instance;
	juce::MidiBuffer midi;
	PluginPlayhead playhead_now;
	double sample_rate = 0.0;
	int input_channels = 0;
	int output_channels = 0;
	int tail = 0;
};
