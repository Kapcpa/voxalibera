#include "ui/panel_view.h"

#include "ui/theme.h"

namespace
{
	constexpr float TITLE_FONT_SIZE = 13.0f;
	constexpr int TITLE_PADDING = 8;
}

PanelView::PanelView(const juce::String& title) : title(title)
{
}

void PanelView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::PANEL_BACKGROUND);

	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawRect(getLocalBounds());

	graphics.setColour(theme::DIM_TEXT);
	graphics.setFont(juce::FontOptions(TITLE_FONT_SIZE));
	graphics.drawText(title, getLocalBounds().reduced(TITLE_PADDING), juce::Justification::topLeft);
}
