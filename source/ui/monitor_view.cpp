#include "ui/monitor_view.h"

#include <array>
#include <utility>

namespace
{
constexpr int ROW_HEIGHT = 28;
constexpr int ROW_GAP = 8;
constexpr int TOGGLE_WIDTH = 230;
constexpr int ROUTING_LABEL_WIDTH = 60;
constexpr int RESCAN_WIDTH = 130;
constexpr int JACK_BUFFER_WIDTH = 160;

const std::array JACK_BUFFER_SIZES{32, 64, 128, 256, 512, 1024};

const std::array ROUTING_CHOICES{
	std::pair{InputRouting::left_to_both, "Input 1 / left -> both outputs"},
	std::pair{InputRouting::right_to_both, "Input 2 / right -> both outputs"},
	std::pair{InputRouting::stereo, "Stereo as-is"}};

int routing_id(InputRouting routing)
{
	for (size_t position = 0; position < ROUTING_CHOICES.size(); ++position)
		if (ROUTING_CHOICES[position].first == routing)
			return (int) position + 1;

	return 0;
}

InputRouting routing_from_id(int id)
{
	return ROUTING_CHOICES[(size_t) id - 1].first;
}
}

MonitorView::MonitorView(AppContext& context)
	: context(context)
{
	monitoring_toggle.setToggleState(context.settings.monitoring, juce::dontSendNotification);
	monitoring_toggle.onClick = [this]
	{ set_monitoring(monitoring_toggle.getToggleState()); };

	for (const auto& [routing, text] : ROUTING_CHOICES)
		routing_box.addItem(text, routing_id(routing));

	routing_box.setSelectedId(routing_id(context.settings.input_routing), juce::dontSendNotification);
	routing_box.onChange = [this]
	{ set_routing(routing_from_id(routing_box.getSelectedId())); };

	for (const auto size : JACK_BUFFER_SIZES)
		jack_buffer_box.addItem(juce::String(size) + " samples", size);

	jack_buffer_box.setSelectedId(context.settings.jack_buffer_size, juce::dontSendNotification);
	jack_buffer_box.onChange = [this]
	{ set_jack_buffer_size(jack_buffer_box.getSelectedId()); };

	rescan_button.onClick = [this]
	{ this->context.devices.rescan_devices(); };

	addAndMakeVisible(monitoring_toggle);
	addAndMakeVisible(routing_label);
	addAndMakeVisible(routing_box);
	addAndMakeVisible(rescan_button);
	addAndMakeVisible(jack_buffer_label);
	addAndMakeVisible(jack_buffer_box);
}

void MonitorView::update()
{
	jack_buffer_box.setEnabled(context.engine.device_manager.getCurrentAudioDeviceType() == DeviceSession::JACK);
}

void MonitorView::set_monitoring(bool monitoring)
{
	context.engine.passthrough.monitoring = monitoring;
	context.settings.monitoring = monitoring;
	context.settings.save();
}

void MonitorView::set_routing(InputRouting routing)
{
	context.engine.passthrough.routing = routing;
	context.settings.input_routing = routing;
	context.settings.save();
}

void MonitorView::set_jack_buffer_size(int buffer_size)
{
	context.settings.jack_buffer_size = buffer_size;
	context.settings.save();
}

void MonitorView::resized()
{
	auto area = getLocalBounds();

	auto routing_row = area.removeFromTop(ROW_HEIGHT);
	monitoring_toggle.setBounds(routing_row.removeFromLeft(TOGGLE_WIDTH));
	routing_label.setBounds(routing_row.removeFromLeft(ROUTING_LABEL_WIDTH));
	rescan_button.setBounds(routing_row.removeFromRight(RESCAN_WIDTH));
	routing_row.removeFromRight(ROW_GAP);
	routing_box.setBounds(routing_row);

	area.removeFromTop(ROW_GAP);

	auto jack_row = area.removeFromTop(ROW_HEIGHT);
	jack_buffer_label.setBounds(jack_row.removeFromLeft(TOGGLE_WIDTH));
	jack_buffer_box.setBounds(jack_row.removeFromLeft(JACK_BUFFER_WIDTH));
}
