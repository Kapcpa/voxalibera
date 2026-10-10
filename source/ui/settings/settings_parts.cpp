#include "ui/settings/settings_parts.h"

#include "ui/theme.h"

namespace
{
	constexpr float SECTION_FONT_SIZE = 12.0f;
	constexpr float SECTION_LETTER_SPACING = 0.08f;
	constexpr int SIDEBAR_TEXT_PADDING = 14;
	constexpr int SIDEBAR_MARKER_WIDTH = 3;
	constexpr float SIDEBAR_FONT_SIZE = 14.0f;
	constexpr float SIDEBAR_CORNER_RADIUS = 4.0f;
}

SectionLabel::SectionLabel(const juce::String& title) : Label({}, title.toUpperCase())
{
	setFont(juce::FontOptions(SECTION_FONT_SIZE, juce::Font::bold).withKerningFactor(SECTION_LETTER_SPACING));
	setColour(juce::Label::textColourId, theme::DIM_TEXT);
	setBorderSize({});
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
		graphics.fillRoundedRectangle(bounds.toFloat(), SIDEBAR_CORNER_RADIUS);
	}

	if (getToggleState()) {
		graphics.setColour(theme::ACCENT);
		graphics.fillRect(bounds.withWidth(SIDEBAR_MARKER_WIDTH));
	}

	graphics.setColour(getToggleState() ? theme::TEXT : theme::DIM_TEXT.brighter(0.4f));
	graphics.setFont(juce::FontOptions(SIDEBAR_FONT_SIZE));
	graphics.drawText(getName(), bounds.withTrimmedLeft(SIDEBAR_TEXT_PADDING), juce::Justification::centredLeft);
}
