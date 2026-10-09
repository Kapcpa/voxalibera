#include "ui/settings/section_label.h"

#include "ui/theme.h"

namespace
{
	constexpr float FONT_SIZE = 12.0f;
	constexpr float LETTER_SPACING = 0.08f;
}

SectionLabel::SectionLabel(const juce::String& title) : Label({}, title.toUpperCase())
{
	setFont(juce::FontOptions(FONT_SIZE, juce::Font::bold).withKerningFactor(LETTER_SPACING));
	setColour(juce::Label::textColourId, theme::DIM_TEXT);
	setBorderSize({});
}
