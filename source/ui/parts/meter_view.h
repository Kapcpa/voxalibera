#pragma once

#include "engine/levels.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class MeterView : public juce::Component
{
public:
	enum class Orientation
	{
		horizontal,
		vertical,
	};

	explicit MeterView(Orientation orientation);

	void show(LevelMeter* new_meter);
	void update();
	void paint(juce::Graphics& graphics) override;

private:
	float decibels(int channel) const;
	void paint_horizontal(juce::Graphics& graphics);
	void paint_vertical(juce::Graphics& graphics);

	Orientation orientation;
	LevelMeter* meter = nullptr;
	std::array<float, LevelMeter::CHANNELS> displayed{};
};
