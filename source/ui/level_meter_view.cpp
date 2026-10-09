#include "ui/level_meter_view.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace
{
constexpr float FLOOR_DB = -60.0f;
constexpr float CLIPPING_DB = -1.0f;
constexpr float HOT_DB = -12.0f;
constexpr float PEAK_DECAY = 0.85f;
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
}

LevelMeterView::LevelMeterView(LevelMeter& meter)
	: meter(meter)
{
}

void LevelMeterView::update()
{
	for (int channel = 0; channel < LevelMeter::CHANNELS; ++channel)
	{
		auto& displayed = displayed_peaks[(size_t) channel];
		displayed = std::max(meter.take_peak(channel), displayed * PEAK_DECAY);
	}

	repaint();
}

void LevelMeterView::paint(juce::Graphics& graphics)
{
	const auto row_height = getHeight() / LevelMeter::CHANNELS;

	for (int channel = 0; channel < LevelMeter::CHANNELS; ++channel)
		paint_channel(graphics, channel, getLocalBounds().withHeight(row_height).withY(channel * row_height).reduced(0, ROW_PADDING));
}

void LevelMeterView::paint_channel(juce::Graphics& graphics, int channel, juce::Rectangle<int> row) const
{
	const auto decibels = juce::Decibels::gainToDecibels(displayed_peaks[(size_t) channel], FLOOR_DB);
	const auto fill = juce::jmap(decibels, FLOOR_DB, 0.0f, 0.0f, 1.0f);
	const auto label_area = row.removeFromLeft(LABEL_WIDTH);
	const auto value_area = row.removeFromRight(VALUE_WIDTH);

	graphics.setColour(juce::Colours::white);
	graphics.drawText("In " + juce::String(channel + 1), label_area, juce::Justification::centredLeft);
	graphics.drawText(level_text(decibels), value_area, juce::Justification::centredRight);

	graphics.setColour(juce::Colours::black.withAlpha(0.4f));
	graphics.fillRect(row);

	graphics.setColour(level_colour(decibels));
	graphics.fillRect(row.withWidth(juce::roundToInt((float) row.getWidth() * fill)));
}
