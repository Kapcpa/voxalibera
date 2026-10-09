#include "app/window_content.h"

WindowContent::WindowContent(juce::MenuBarModel& menu, MainView& main_view, ModalOverlay& overlay) : menu_bar(&menu), main_view(main_view), overlay(overlay)
{
	addAndMakeVisible(menu_bar);
	addAndMakeVisible(main_view);
	addChildComponent(overlay);
	setSize(main_view.getWidth(), main_view.getHeight() + getLookAndFeel().getDefaultMenuBarHeight());
}

void WindowContent::resized()
{
	auto area = getLocalBounds();
	overlay.setBounds(area);
	menu_bar.setBounds(area.removeFromTop(getLookAndFeel().getDefaultMenuBarHeight()));
	main_view.setBounds(area);
}
