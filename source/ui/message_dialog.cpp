#include "ui/message_dialog.h"

#include "ui/settings/settings_parts.h"
#include "ui/theme.h"

namespace
{
	constexpr int WIDTH = 480;
	constexpr int MIN_MESSAGE_HEIGHT = 40;
	constexpr float MESSAGE_FONT_SIZE = 15.0f;
}

MessageDialog::MessageDialog()
{
	message_label.setFont(juce::FontOptions(MESSAGE_FONT_SIZE));
	message_label.setColour(juce::Label::textColourId, theme::TEXT);
	message_label.setJustificationType(juce::Justification::topLeft);
	message_label.setBorderSize({});
	addAndMakeVisible(message_label);
}

void MessageDialog::prepare(const juce::String& message, std::vector<MessageChoice> new_choices)
{
	using namespace settings_layout;

	choices = std::move(new_choices);
	buttons.clear();

	for (size_t index = 0; index < choices.size(); ++index) {
		auto& button = *buttons.emplace_back(std::make_unique<juce::TextButton>(choices[index].text));
		button.onClick = [this, index] { choose(index); };
		addAndMakeVisible(button);
	}

	message_label.setText(message, juce::dontSendNotification);

	juce::AttributedString text;
	text.append(message, juce::FontOptions(MESSAGE_FONT_SIZE));
	juce::TextLayout layout;
	layout.createLayout(text, (float) (WIDTH - 2 * PAGE_PADDING));

	const auto message_height = std::max(MIN_MESSAGE_HEIGHT, (int) std::ceil(layout.getHeight()));
	setSize(WIDTH, PAGE_PADDING + message_height + PAGE_PADDING + ROW_HEIGHT + PAGE_PADDING);
	resized();
}

void MessageDialog::choose(size_t index)
{
	const auto action = choices[index].action;

	if (on_finished)
		on_finished();

	if (action)
		action();
}

void MessageDialog::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds().reduced(PAGE_PADDING);
	auto row = area.removeFromBottom(ROW_HEIGHT);

	for (auto button = buttons.rbegin(); button != buttons.rend(); ++button) {
		(*button)->setBounds(row.removeFromRight(BUTTON_WIDTH));
		row.removeFromRight(ROW_GAP);
	}

	area.removeFromBottom(PAGE_PADDING);
	message_label.setBounds(area);
}
