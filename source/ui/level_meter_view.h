#pragma once

#include "engine/level_meter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class LevelMeterView : public juce::Component
{
public:
	explicit LevelMeterView(LevelMeter& meter);

	void update();
	void paint(juce::Graphics& graphics) override;

private:
	void paint_channel(juce::Graphics& graphics, int channel, juce::Rectangle<int> row) const;

	LevelMeter& meter;
	std::array<float, LevelMeter::CHANNELS> displayed_peaks{};
};
