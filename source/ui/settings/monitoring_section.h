#pragma once

#include "core/app_context.h"
#include "ui/level_meter_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MonitoringSection : public juce::Component
{
public:
	explicit MonitoringSection(AppContext& context);

	void update();
	void resized() override;

	static int preferred_height();

private:
	void set_monitoring(bool monitoring);
	void set_routing(InputRouting routing);

	AppContext& context;
	juce::Label monitoring_label{{}, "Monitor"};
	juce::ToggleButton monitoring_toggle{"Hear the input through the output"};
	juce::Label routing_label{{}, "Source"};
	juce::ComboBox routing_box;
	juce::Label meter_label{{}, "Input level"};
	LevelMeterView level_meter_view;
};
