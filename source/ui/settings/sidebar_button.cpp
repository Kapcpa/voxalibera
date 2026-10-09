#include "ui/settings/sidebar_button.h"

#include "ui/theme.h"

namespace
{
	constexpr int TEXT_PADDING = 14;
	constexpr int MARKER_WIDTH = 3;
	constexpr float FONT_SIZE = 14.0f;
	constexpr float CORNER_RADIUS = 4.0f;
}

SidebarButton::SidebarButton(const juce::String& name) : Button(name)
{
	setClickingTogglesState(true);
}

void SidebarButton::paintButton(juce::Graphics& graphics, bool highlighted, bool)
{
	const auto bounds = getLocalBounds();

	if (getToggleState() || highlighted) {
		graphics.setColour(getToggleState() ? theme::WIDGET_BACKGROUND : theme::PANEL_BACKGROUND);
		graphics.fillRoundedRectangle(bounds.toFloat(), CORNER_RADIUS);
	}

	if (getToggleState()) {
		graphics.setColour(theme::ACCENT);
		graphics.fillRect(bounds.withWidth(MARKER_WIDTH));
	}

	graphics.setColour(getToggleState() ? theme::TEXT : theme::DIM_TEXT.brighter(0.4f));
	graphics.setFont(juce::FontOptions(FONT_SIZE));
	graphics.drawText(getName(), bounds.withTrimmedLeft(TEXT_PADDING), juce::Justification::centredLeft);
}
