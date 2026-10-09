#pragma once

#include "core/app_context.h"
#include "ui/diagnostics_view.h"
#include "ui/hardware_controls_view.h"
#include "ui/level_meter_view.h"
#include "ui/monitor_view.h"

#include <juce_audio_utils/juce_audio_utils.h>

class AudioSettingsView : public juce::Component
{
public:
	explicit AudioSettingsView(AppContext& context);

	void update();

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	juce::AudioDeviceSelectorComponent device_selector;
	MonitorView monitor_view;
	LevelMeterView level_meter_view;
	DiagnosticsView diagnostics_view;
	HardwareControlsView hardware_controls_view;
};
