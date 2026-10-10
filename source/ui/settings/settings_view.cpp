#include "ui/settings/settings_view.h"

#include "ui/settings/settings_parts.h"
#include "ui/theme.h"

namespace
{
	constexpr int WIDTH = 900;
	constexpr int HEIGHT = 680;
	constexpr int SIDEBAR_WIDTH = 180;
	constexpr int SIDEBAR_PADDING = 12;
	constexpr int SIDEBAR_BUTTON_HEIGHT = 32;
	constexpr int SIDEBAR_BUTTON_GAP = 4;
	constexpr int SIDEBAR_RADIO_GROUP = 1;
}

SettingsView::SettingsView(AppContext& context) : audio_page(context), interface_page(context)
{
	for (size_t position = 0; position < pages.size(); ++position) {
		buttons[position]->setRadioGroupId(SIDEBAR_RADIO_GROUP);
		buttons[position]->onClick = [this, position] { show_page((int) position); };
		addAndMakeVisible(buttons[position]);
		addChildComponent(pages[position]);
	}

	show_page(AUDIO_PAGE);
	setSize(WIDTH, HEIGHT);
}

void SettingsView::show_page(int page)
{
	current_page = page;

	for (size_t position = 0; position < pages.size(); ++position) {
		const auto selected = (int) position == page;
		pages[position]->setVisible(selected);
		buttons[position]->setToggleState(selected, juce::dontSendNotification);
	}
}

void SettingsView::update()
{
	pages[(size_t) current_page]->update();
}

void SettingsView::paint(juce::Graphics& graphics)
{
	const auto sidebar = getLocalBounds().removeFromLeft(SIDEBAR_WIDTH);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawVerticalLine(sidebar.getRight(), (float) sidebar.getY(), (float) sidebar.getBottom());
}

void SettingsView::resized()
{
	auto area = getLocalBounds();
	auto sidebar = area.removeFromLeft(SIDEBAR_WIDTH).reduced(SIDEBAR_PADDING);

	for (auto* button : buttons) {
		button->setBounds(sidebar.removeFromTop(SIDEBAR_BUTTON_HEIGHT));
		sidebar.removeFromTop(SIDEBAR_BUTTON_GAP);
	}

	for (auto* page : pages)
		page->setBounds(area.reduced(settings_layout::PAGE_PADDING));
}
