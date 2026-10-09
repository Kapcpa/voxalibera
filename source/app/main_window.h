#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MainWindow : public juce::DocumentWindow
{
public:
	MainWindow(const juce::String& name, AppContext& context);

	void closeButtonPressed() override;
};
