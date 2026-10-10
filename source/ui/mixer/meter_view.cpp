#include "ui/mixer/meter_view.h"

#include "ui/theme.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace
{
	constexpr float FLOOR_DB = -60.0f;
	constexpr float CLIPPING_DB = -1.0f;
	constexpr float HOT_DB = -12.0f;
	constexpr float PEAK_DECAY = 0.85f;
	constexpr int CHANNEL_GAP = 1;

	juce::Colour level_colour(float decibels)
	{
		if (decibels > CLIPPING_DB)
			return juce::Colours::red;

		if (decibels > HOT_DB)
			return juce::Colours::orange;

		return juce::Colours::limegreen;
	}
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

void MeterView::paint(juce::Graphics& graphics)
{
	const auto channel_width = (getWidth() - CHANNEL_GAP) / LevelMeter::CHANNELS;

	for (int channel = 0; channel < LevelMeter::CHANNELS; ++channel) {
		auto column = juce::Rectangle<int>(channel * (channel_width + CHANNEL_GAP), 0, channel_width, getHeight());
		const auto decibels = juce::Decibels::gainToDecibels(displayed[(size_t) channel], FLOOR_DB);
		const auto fill = juce::jmap(decibels, FLOOR_DB, 0.0f, 0.0f, 1.0f);

		graphics.setColour(theme::WINDOW_BACKGROUND);
		graphics.fillRect(column);
		graphics.setColour(level_colour(decibels));
		graphics.fillRect(column.removeFromBottom(juce::roundToInt((float) column.getHeight() * fill)));
	}
}
