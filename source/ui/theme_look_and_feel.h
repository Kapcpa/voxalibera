#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ThemeLookAndFeel : public juce::LookAndFeel_V4
{
public:
	static constexpr int SLIDER_END_MARGIN = 5;

	ThemeLookAndFeel();

	int getSliderThumbRadius(juce::Slider& slider) override;

	void drawLinearSlider(juce::Graphics& graphics, int x, int y, int width, int height, float slider_position, float minimum_position, float maximum_position, juce::Slider::SliderStyle style, juce::Slider& slider) override;
	void drawRotarySlider(juce::Graphics& graphics, int x, int y, int width, int height, float position, float start_angle, float end_angle, juce::Slider& slider) override;
};
