#pragma once

#include "core/app_context.h"
#include "ui/diagnostics_view.h"
#include "ui/hardware_controls_view.h"
#include "ui/level_meter_view.h"
#include "ui/monitor_view.h"

#include <juce_audio_utils/juce_audio_utils.h>

class MainView : public juce::Component, private juce::Timer
{
public:
	explicit MainView(AppContext& context);

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	void timerCallback() override;

	AppContext& context;
	juce::AudioDeviceSelectorComponent device_selector;
	MonitorView monitor_view;
	LevelMeterView level_meter_view;
	DiagnosticsView diagnostics_view;
	HardwareControlsView hardware_controls_view;
};
