#pragma once

#include "core/app_context.h"
#include "ui/audio_settings_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

class SettingsWindow : public juce::DocumentWindow
{
public:
	explicit SettingsWindow(AppContext& context);

	void update();
	void closeButtonPressed() override;

private:
	AudioSettingsView audio_settings_view;
};
