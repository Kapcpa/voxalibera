#include "ui/settings/status_section.h"

#include "ui/settings/settings_parts.h"
#include "ui/theme.h"

namespace
{
	constexpr int FRAMES_PER_REFRESH = 10;
	constexpr int COLUMNS = 2;
	constexpr int ROW_HEIGHT = 22;
	constexpr int NAME_WIDTH = 120;
	constexpr int UNREPORTED_XRUNS = -1;
	constexpr auto NOT_APPLICABLE = "-";

	const std::array<juce::String, StatusSection::ITEM_COUNT> NAMES{"Device", "Driver", "Sample rate", "Buffer", "Latency", "Access", "CPU", "Audio thread", "Xruns", "Problems"};

	double to_milliseconds(int samples, double sample_rate)
	{
		return sample_rate > 0.0 ? 1000.0 * samples / sample_rate : 0.0;
	}

	juce::String describe_xruns(int xruns)
	{
		return xruns == UNREPORTED_XRUNS ? juce::String("not reported") : juce::String(xruns);
	}
}

StatusSection::StatusSection(AppContext& context) : context(context)
{
	for (size_t position = 0; position < items.size(); ++position) {
		auto& item = items[position];
		item.name.setText(NAMES[position], juce::dontSendNotification);
		item.name.setColour(juce::Label::textColourId, theme::DIM_TEXT);
		addAndMakeVisible(item.name);
		addAndMakeVisible(item.value);
	}
}

int StatusSection::preferred_height()
{
	return (ITEM_COUNT / COLUMNS) * ROW_HEIGHT;
}

void StatusSection::update()
{
	if (--frames_until_refresh > 0)
		return;

	frames_until_refresh = FRAMES_PER_REFRESH;
	const auto values = context.engine.device_manager.getCurrentAudioDevice() != nullptr ? describe() : describe_missing_device();

	for (size_t position = 0; position < items.size(); ++position)
		items[position].value.setText(values[position], juce::dontSendNotification);
}

std::array<juce::String, StatusSection::ITEM_COUNT> StatusSection::describe() const
{
	auto& device = *context.engine.device_manager.getCurrentAudioDevice();
	const auto rate = device.getCurrentSampleRate();
	const auto buffer = device.getCurrentBufferSizeSamples();
	const auto latency = device.getInputLatencyInSamples() + device.getOutputLatencyInSamples();
	const auto is_jack = device.getTypeName() == DeviceSession::JACK;
	const auto access = context.devices.is_exclusive() ? "Exclusive (borrowed from PipeWire)" : (is_jack ? "Shared through PipeWire" : "Shared");
	const auto problems = context.devices.reservation_problems.trim();

	return {
		device.getName(),
		device.getTypeName(),
		juce::String(rate, 0) + " Hz",
		juce::String(buffer) + " samples (" + juce::String(to_milliseconds(buffer, rate), 1) + " ms)",
		"~" + juce::String(to_milliseconds(latency, rate), 1) + " ms round trip",
		access,
		juce::String(context.engine.device_manager.getCpuUsage() * 100.0, 1) + " %",
		context.engine.running_realtime ? "Real-time" : "Normal priority (expect pops)",
		describe_xruns(device.getXRunCount()),
		problems.isEmpty() ? juce::String("None") : problems,
	};
}

std::array<juce::String, StatusSection::ITEM_COUNT> StatusSection::describe_missing_device() const
{
	auto values = std::array<juce::String, ITEM_COUNT>{};
	values.fill(NOT_APPLICABLE);
	values[0] = "No audio device open";
	return values;
}

void StatusSection::resized()
{
	const auto column_width = getWidth() / COLUMNS;
	const auto rows_per_column = ITEM_COUNT / COLUMNS;

	for (size_t position = 0; position < items.size(); ++position) {
		const auto column = (int) position / rows_per_column;
		const auto row = (int) position % rows_per_column;
		auto bounds = juce::Rectangle<int>(column * column_width, row * ROW_HEIGHT, column_width, ROW_HEIGHT);
		items[position].name.setBounds(bounds.removeFromLeft(NAME_WIDTH));
		items[position].value.setBounds(bounds);
	}
}
