#pragma once

#include "ui/main_view.h"
#include "ui/modal_overlay.h"

#include <juce_gui_basics/juce_gui_basics.h>

class WindowContent : public juce::Component
{
public:
	WindowContent(juce::MenuBarModel& menu, MainView& main_view, ModalOverlay& overlay);

	void resized() override;

private:
	juce::MenuBarComponent menu_bar;
	MainView& main_view;
	ModalOverlay& overlay;
};
