#pragma once

#include "engine/level_meter.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class MeterView : public juce::Component
{
public:
	void show(LevelMeter* new_meter);
	void update();
	void paint(juce::Graphics& graphics) override;

private:
	LevelMeter* meter = nullptr;
	std::array<float, LevelMeter::CHANNELS> displayed{};
};
