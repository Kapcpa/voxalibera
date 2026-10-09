#pragma once

#include "core/app_context.h"
#include "ui/device_selector_view.h"
#include "ui/diagnostics_view.h"
#include "ui/hardware_controls_view.h"
#include "ui/level_meter_view.h"
#include "ui/monitor_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

class AudioSettingsView : public juce::Component
{
public:
	explicit AudioSettingsView(AppContext& context);

	void update();

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	DeviceSelectorView device_selector;
	MonitorView monitor_view;
	LevelMeterView level_meter_view;
	DiagnosticsView diagnostics_view;
	HardwareControlsView hardware_controls_view;
};
