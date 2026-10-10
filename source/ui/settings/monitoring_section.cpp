#include "ui/settings/monitoring_section.h"

#include "ui/settings/settings_layout.h"

#include <array>
#include <utility>

namespace
{
	constexpr int METER_HEIGHT = 52;

	const std::array ROUTING_CHOICES{
		std::pair{InputRouting::left_to_both, "Input 1 (left) to both sides"},
		std::pair{InputRouting::right_to_both, "Input 2 (right) to both sides"},
		std::pair{InputRouting::stereo, "Stereo as it comes in"},
	};

	int routing_index(InputRouting routing)
	{
		for (size_t position = 0; position < ROUTING_CHOICES.size(); ++position)
			if (ROUTING_CHOICES[position].first == routing)
				return (int) position;

		return 0;
	}
}

MonitoringSection::MonitoringSection(AppContext& context) : context(context), level_meter_view(context.engine.input_meter)
{
	monitoring_toggle.setToggleState(context.settings.test_monitor, juce::dontSendNotification);
	monitoring_toggle.onClick = [this] { set_monitoring(monitoring_toggle.getToggleState()); };

	for (const auto& [routing, text] : ROUTING_CHOICES)
		routing_box.addItem(text, routing_index(routing) + 1);

	routing_box.setSelectedItemIndex(routing_index(context.settings.input_routing), juce::dontSendNotification);
	routing_box.onChange = [this] { set_routing(ROUTING_CHOICES[(size_t) routing_box.getSelectedItemIndex()].first); };

	addAndMakeVisible(monitoring_label);
	addAndMakeVisible(monitoring_toggle);
	addAndMakeVisible(routing_label);
	addAndMakeVisible(routing_box);
	addAndMakeVisible(meter_label);
	addAndMakeVisible(level_meter_view);
}

int MonitoringSection::preferred_height()
{
	return 2 * (settings_layout::ROW_HEIGHT + settings_layout::ROW_GAP) + METER_HEIGHT;
}

void MonitoringSection::update()
{
	level_meter_view.update();
}

void MonitoringSection::set_monitoring(bool monitoring)
{
	context.engine.passthrough.monitoring = monitoring;
	context.settings.test_monitor = monitoring;
	context.settings.save();
}

void MonitoringSection::set_routing(InputRouting routing)
{
	context.engine.passthrough.routing = routing;
	context.settings.input_routing = routing;
	context.settings.save();
}

void MonitoringSection::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds();

	auto monitoring_row = area.removeFromTop(ROW_HEIGHT);
	monitoring_label.setBounds(monitoring_row.removeFromLeft(LABEL_WIDTH));
	monitoring_toggle.setBounds(monitoring_row.removeFromLeft(FIELD_WIDTH));
	area.removeFromTop(ROW_GAP);

	auto routing_row = area.removeFromTop(ROW_HEIGHT);
	routing_label.setBounds(routing_row.removeFromLeft(LABEL_WIDTH));
	routing_box.setBounds(routing_row.removeFromLeft(FIELD_WIDTH));
	area.removeFromTop(ROW_GAP);

	auto meter_row = area.removeFromTop(METER_HEIGHT);
	meter_label.setBounds(meter_row.removeFromLeft(LABEL_WIDTH).withHeight(ROW_HEIGHT));
	level_meter_view.setBounds(meter_row);
}
