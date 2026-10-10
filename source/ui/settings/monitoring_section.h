#pragma once

#include "core/app_context.h"
#include "ui/meter_view.h"

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
	juce::Label monitoring_label{{}, "Test monitor"};
	juce::ToggleButton monitoring_toggle{"Input straight to output, bypassing tracks"};
	juce::Label routing_label{{}, "Source"};
	juce::ComboBox routing_box;
	juce::Label meter_label{{}, "Input level"};
	MeterView level_meter_view{MeterView::Orientation::horizontal};
};
