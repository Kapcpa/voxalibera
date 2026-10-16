#include "ui/plugins/builtin_editor.h"

#include "ui/theme.h"

#include <algorithm>

namespace
{
	constexpr int MAX_COLUMNS = 4;
	constexpr int MIN_WIDTH = 260;
	constexpr int HEADER_HEIGHT = 40;
	constexpr int PADDING = 16;
	constexpr int KNOB_WIDTH = 88;
	constexpr int KNOB_GAP = 8;
	constexpr int NAME_HEIGHT = 18;
	constexpr int DIAL_HEIGHT = 64;
	constexpr int VALUE_HEIGHT = 20;
	constexpr int KNOB_HEIGHT = NAME_HEIGHT + DIAL_HEIGHT + VALUE_HEIGHT;
	constexpr float TITLE_FONT_SIZE = 15.0f;
	constexpr float NAME_FONT_SIZE = 12.0f;
	constexpr float PANEL_CORNER_RADIUS = 6.0f;
	constexpr auto EMPTY_TEXT = "This effect has no settings";
}

BuiltinEditor::BuiltinEditor(BuiltinProcessor& processor, const juce::String& title) : processor(processor), title(title)
{
	for (const auto& parameter : processor.parameters())
		add_knob(*parameter);

	const auto columns = column_count();
	const auto rows = std::max(1, ((int) knobs.size() + columns - 1) / columns);
	const auto width = std::max(MIN_WIDTH, columns * KNOB_WIDTH + (columns - 1) * KNOB_GAP + 4 * PADDING);
	const auto height = HEADER_HEIGHT + rows * KNOB_HEIGHT + (rows - 1) * KNOB_GAP + 4 * PADDING;
	setSize(width, height);
}

void BuiltinEditor::add_knob(BuiltinParameter& parameter)
{
	auto& knob = *knobs.emplace_back(std::make_unique<Knob>());
	const auto& info = parameter.info;

	knob.name.setText(info.name, juce::dontSendNotification);
	knob.name.setFont(juce::FontOptions(NAME_FONT_SIZE, juce::Font::bold));
	knob.name.setJustificationType(juce::Justification::centred);
	knob.name.setColour(juce::Label::textColourId, theme::DIM_TEXT.brighter(0.5f));

	knob.slider.setRange(info.minimum, info.maximum);

	if (info.centre)
		knob.slider.setSkewFactorFromMidPoint(*info.centre);

	knob.slider.setValue(parameter.value(), juce::dontSendNotification);
	knob.slider.setDoubleClickReturnValue(true, info.default_value);
	knob.slider.setNumDecimalPlacesToDisplay(info.decimals);
	knob.slider.setTextValueSuffix(info.unit.isEmpty() ? juce::String() : " " + info.unit);
	knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, KNOB_WIDTH, VALUE_HEIGHT);
	knob.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
	knob.slider.setScrollWheelEnabled(false);
	knob.slider.onValueChange = [this, &knob, &parameter] { this->processor.set_parameter(parameter, (float) knob.slider.getValue()); };

	addAndMakeVisible(knob.name);
	addAndMakeVisible(knob.slider);
}

int BuiltinEditor::column_count() const
{
	return std::clamp((int) knobs.size(), 1, MAX_COLUMNS);
}

void BuiltinEditor::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);

	auto area = getLocalBounds();
	const auto header = area.removeFromTop(HEADER_HEIGHT);

	graphics.setColour(theme::PANEL_BACKGROUND);
	graphics.fillRect(header);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(header.getBottom() - 1, 0.0f, (float) getWidth());
	graphics.setColour(theme::TEXT);
	graphics.setFont(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold));
	graphics.drawText(title, header.reduced(PADDING, 0), juce::Justification::centredLeft, true);

	const auto panel = area.reduced(PADDING).toFloat();
	graphics.setColour(theme::PANEL_BACKGROUND);
	graphics.fillRoundedRectangle(panel, PANEL_CORNER_RADIUS);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawRoundedRectangle(panel, PANEL_CORNER_RADIUS, 1.0f);

	if (!knobs.empty())
		return;

	graphics.setColour(theme::DIM_TEXT);
	graphics.setFont(juce::FontOptions(NAME_FONT_SIZE));
	graphics.drawText(EMPTY_TEXT, panel, juce::Justification::centred, false);
}

void BuiltinEditor::resized()
{
	const auto columns = column_count();
	const auto row_width = columns * KNOB_WIDTH + (columns - 1) * KNOB_GAP;
	const auto content = getLocalBounds().withTrimmedTop(HEADER_HEIGHT).reduced(2 * PADDING);
	const auto left = content.getX() + (content.getWidth() - row_width) / 2;

	for (size_t index = 0; index < knobs.size(); ++index) {
		const auto column = (int) index % columns;
		const auto row = (int) index / columns;
		auto cell = juce::Rectangle<int>(left + column * (KNOB_WIDTH + KNOB_GAP), content.getY() + row * (KNOB_HEIGHT + KNOB_GAP), KNOB_WIDTH, KNOB_HEIGHT);

		knobs[index]->name.setBounds(cell.removeFromTop(NAME_HEIGHT));
		knobs[index]->slider.setBounds(cell);
	}
}
