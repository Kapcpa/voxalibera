#include "ui/parts/plugins_button.h"

#include "ui/theme.h"

namespace
{
	constexpr auto TEXT = "FX";
}

PluginsButton::PluginsButton() : juce::TextButton(TEXT)
{
	setColour(juce::TextButton::textColourOnId, theme::WINDOW_BACKGROUND);
	setWantsKeyboardFocus(false);
}

void PluginsButton::show(const Track& track)
{
	setColour(juce::TextButton::buttonOnColourId, track.plugins_bypassed ? theme::BYPASS : theme::ACCENT);
	setToggleState(!track.plugins.empty(), juce::dontSendNotification);
}
