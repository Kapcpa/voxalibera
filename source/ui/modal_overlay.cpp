#include "ui/modal_overlay.h"

#include "ui/theme.h"

namespace
{
	constexpr int SCREEN_MARGIN = 32;
	constexpr int HEADER_HEIGHT = 44;
	constexpr int HEADER_PADDING = 16;
	constexpr int CLOSE_BUTTON_WIDTH = 80;
	constexpr int CLOSE_BUTTON_HEIGHT = 26;
	constexpr int CORNER_RADIUS = 8;
	constexpr float TITLE_FONT_SIZE = 16.0f;
}

ModalOverlay::ModalOverlay()
{
	close_button.onClick = [this] { close(); };
	addAndMakeVisible(close_button);
	setWantsKeyboardFocus(true);
}

void ModalOverlay::open(const juce::String& new_title, juce::Component& new_content, bool with_close_button)
{
	close_button.setVisible(with_close_button);
	if (content != nullptr)
		removeChildComponent(content);

	title = new_title;
	content = &new_content;
	content_size = new_content.getLocalBounds();
	addAndMakeVisible(new_content);
	resized();
	setVisible(true);
	toFront(true);
	grabKeyboardFocus();
}

void ModalOverlay::close()
{
	setVisible(false);

	if (on_close)
		on_close();
}

juce::Rectangle<int> ModalOverlay::dialog_bounds() const
{
	const auto available = getLocalBounds().reduced(SCREEN_MARGIN);
	const auto width = std::min(content_size.getWidth(), available.getWidth());
	const auto height = std::min(content_size.getHeight() + HEADER_HEIGHT, available.getHeight());
	return juce::Rectangle<int>(width, height).withCentre(getLocalBounds().getCentre());
}

void ModalOverlay::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::BACKDROP);

	const auto dialog = dialog_bounds().toFloat();
	graphics.setColour(theme::PANEL_BACKGROUND);
	graphics.fillRoundedRectangle(dialog, CORNER_RADIUS);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawRoundedRectangle(dialog, CORNER_RADIUS, 1.0f);

	graphics.setColour(theme::TEXT);
	graphics.setFont(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold));
	graphics.drawText(title, dialog_bounds().removeFromTop(HEADER_HEIGHT).reduced(HEADER_PADDING, 0), juce::Justification::centredLeft);
}

void ModalOverlay::resized()
{
	auto dialog = dialog_bounds();
	auto header = dialog.removeFromTop(HEADER_HEIGHT).reduced(HEADER_PADDING, 0);

	close_button.setBounds(header.removeFromRight(CLOSE_BUTTON_WIDTH).withSizeKeepingCentre(CLOSE_BUTTON_WIDTH, CLOSE_BUTTON_HEIGHT));
	if (content != nullptr)
		content->setBounds(dialog);
}

bool ModalOverlay::keyPressed(const juce::KeyPress& key)
{
	if (key == juce::KeyPress::escapeKey)
		close();

	return true;
}
