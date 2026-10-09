#include "ui/diagnostics_view.h"

namespace
{
	constexpr int FRAMES_PER_REFRESH = 10;
	constexpr float FONT_SIZE = 14.0f;
	constexpr int UNREPORTED_XRUNS = -1;

	double samples_to_milliseconds(int samples, double sample_rate)
	{
		return sample_rate > 0.0 ? 1000.0 * samples / sample_rate : 0.0;
	}

	juce::String describe_xruns(int xruns)
	{
		return xruns == UNREPORTED_XRUNS ? juce::String("not reported") : juce::String(xruns);
	}
}

DiagnosticsView::DiagnosticsView(AppContext& context) : context(context)
{
	text.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), FONT_SIZE, juce::Font::plain));
	text.setJustificationType(juce::Justification::topLeft);
	addAndMakeVisible(text);
}

void DiagnosticsView::update()
{
	if (--frames_until_refresh > 0)
		return;

	frames_until_refresh = FRAMES_PER_REFRESH;
	auto* device = context.engine.device_manager.getCurrentAudioDevice();
	auto lines = device != nullptr ? describe_device(*device) : juce::StringArray{"No audio device open."};

	if (context.devices.reservation_problems.isNotEmpty())
		lines.add("Problem:   " + context.devices.reservation_problems.trim());

	text.setText(lines.joinIntoString("\n"), juce::dontSendNotification);
}

juce::StringArray DiagnosticsView::describe_device(juce::AudioIODevice& device) const
{
	const auto sample_rate = device.getCurrentSampleRate();
	const auto buffer_size = device.getCurrentBufferSizeSamples();
	const auto input_latency = device.getInputLatencyInSamples();
	const auto output_latency = device.getOutputLatencyInSamples();
	const auto is_jack = device.getTypeName() == DeviceSession::JACK;

	juce::StringArray lines;
	lines.add("Device:    " + device.getTypeName() + " / " + device.getName());
	lines.add("Rate:      " + juce::String(sample_rate, 0) + " Hz");
	lines.add("Buffer:    " + juce::String(buffer_size) + " samples (" + juce::String(samples_to_milliseconds(buffer_size, sample_rate), 2) + " ms)");
	lines.add("Latency:   in " + juce::String(input_latency) + " + out " + juce::String(output_latency) + " samples = ~" + juce::String(samples_to_milliseconds(input_latency + output_latency, sample_rate), 2) + " ms (as reported)");
	lines.add("Exclusive: " + juce::String(context.devices.is_exclusive() ? "yes, borrowed from PipeWire" : "no, shared through PipeWire"));

	if (is_jack)
		lines.add("PipeWire:  " + context.devices.jack_properties + " while connected");

	lines.add("CPU:       " + juce::String(context.engine.device_manager.getCpuUsage() * 100.0, 1) + " %");
	lines.add("Thread:    " + juce::String(context.engine.running_realtime ? "real-time" : "NORMAL priority (expect pops)"));
	lines.add("Xruns:     " + describe_xruns(device.getXRunCount()));
	return lines;
}

void DiagnosticsView::resized()
{
	text.setBounds(getLocalBounds());
}
