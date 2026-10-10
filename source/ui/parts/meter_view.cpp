#include "ui/parts/meter_view.h"

#include "ui/theme.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace
{
	constexpr float FLOOR_DB = -60.0f;
	constexpr float CLIPPING_DB = -1.0f;
	constexpr float HOT_DB = -12.0f;
	constexpr float PEAK_DECAY = 0.85f;
	constexpr int CHANNEL_GAP = 1;
	constexpr int LABEL_WIDTH = 70;
	constexpr int VALUE_WIDTH = 80;
	constexpr int ROW_PADDING = 4;

	juce::Colour level_colour(float decibels)
	{
		if (decibels > CLIPPING_DB)
			return juce::Colours::red;

		if (decibels > HOT_DB)
			return juce::Colours::orange;

		return juce::Colours::limegreen;
	}

	juce::String level_text(float decibels)
	{
		return decibels <= FLOOR_DB ? juce::String("-inf dB") : juce::String(decibels, 1) + " dB";
	}

	float fill_of(float decibels)
	{
		return juce::jmap(decibels, FLOOR_DB, 0.0f, 0.0f, 1.0f);
	}
}

MeterView::MeterView(Orientation orientation) : orientation(orientation)
{
}

void MeterView::show(LevelMeter* new_meter)
{
	meter = new_meter;
}

void MeterView::update()
{
	for (int channel = 0; channel < LevelMeter::CHANNELS; ++channel) {
		const auto peak = meter != nullptr ? meter->take_peak(channel) : 0.0f;
		auto& level = displayed[(size_t) channel];
		level = std::max(peak, level * PEAK_DECAY);
	}

	repaint();
}

float MeterView::decibels(int channel) const
{
	return juce::Decibels::gainToDecibels(displayed[(size_t) channel], FLOOR_DB);
}

void MeterView::paint(juce::Graphics& graphics)
{
	if (orientation == Orientation::horizontal)
		paint_horizontal(graphics);
	else
		paint_vertical(graphics);
}

void MeterView::paint_horizontal(juce::Graphics& graphics)
{
	const auto row_height = getHeight() / LevelMeter::CHANNELS;

	for (int channel = 0; channel < LevelMeter::CHANNELS; ++channel) {
		auto row = getLocalBounds().withHeight(row_height).withY(channel * row_height).reduced(0, ROW_PADDING);
		const auto label_area = row.removeFromLeft(LABEL_WIDTH);
		const auto value_area = row.removeFromRight(VALUE_WIDTH);
		const auto level = decibels(channel);

		graphics.setColour(theme::TEXT);
		graphics.drawText("In " + juce::String(channel + 1), label_area, juce::Justification::centredLeft);
		graphics.drawText(level_text(level), value_area, juce::Justification::centredRight);

		graphics.setColour(theme::GROOVE);
		graphics.fillRect(row);
		graphics.setColour(level_colour(level));
		graphics.fillRect(row.withWidth(juce::roundToInt((float) row.getWidth() * fill_of(level))));
	}
}

void MeterView::paint_vertical(juce::Graphics& graphics)
{
	const auto column_width = (getWidth() - CHANNEL_GAP) / LevelMeter::CHANNELS;

	for (int channel = 0; channel < LevelMeter::CHANNELS; ++channel) {
		auto column = juce::Rectangle<int>(channel * (column_width + CHANNEL_GAP), 0, column_width, getHeight());
		const auto level = decibels(channel);

		graphics.setColour(theme::GROOVE);
		graphics.fillRect(column);
		graphics.setColour(level_colour(level));
		graphics.fillRect(column.removeFromBottom(juce::roundToInt((float) column.getHeight() * fill_of(level))));
	}
}
