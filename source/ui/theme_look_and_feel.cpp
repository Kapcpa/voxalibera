#include "ui/theme_look_and_feel.h"

#include "ui/theme.h"

namespace
{
	juce::LookAndFeel_V4::ColourScheme theme_colours()
	{
		return {theme::WINDOW_BACKGROUND, theme::WIDGET_BACKGROUND, theme::MENU_BACKGROUND, theme::PANEL_BORDER, theme::TEXT, theme::ACCENT, theme::ACCENT_TEXT, theme::ACCENT, theme::TEXT};
	}
}

ThemeLookAndFeel::ThemeLookAndFeel() : LookAndFeel_V4(theme_colours())
{
	setColour(juce::ResizableWindow::backgroundColourId, theme::WINDOW_BACKGROUND);
	setColour(juce::DocumentWindow::backgroundColourId, theme::WINDOW_BACKGROUND);
	setColour(juce::ComboBox::backgroundColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::ComboBox::outlineColourId, theme::PANEL_BORDER);
	setColour(juce::TextButton::buttonColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::ListBox::backgroundColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::ListBox::outlineColourId, theme::PANEL_BORDER);
	setColour(juce::Slider::trackColourId, theme::ACCENT);
	setColour(juce::Slider::thumbColourId, theme::TEXT);
	setColour(juce::Slider::backgroundColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::Slider::textBoxOutlineColourId, theme::PANEL_BORDER);
	setColour(juce::ScrollBar::thumbColourId, theme::PANEL_BORDER);
	setColour(juce::PopupMenu::backgroundColourId, theme::MENU_BACKGROUND);
	setColour(juce::PopupMenu::highlightedBackgroundColourId, theme::ACCENT);
}
