#pragma once

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace theme
{
	inline const juce::Colour WINDOW_BACKGROUND{0xff1e1f22};
	inline const juce::Colour PANEL_BACKGROUND{0xff26282c};
	inline const juce::Colour PANEL_BORDER{0xff34373c};
	inline const juce::Colour WIDGET_BACKGROUND{0xff303237};
	inline const juce::Colour MENU_BACKGROUND{0xff2b2d31};
	inline const juce::Colour TEXT{0xffdfe1e5};
	inline const juce::Colour DIM_TEXT{0xff6c7078};
	inline const juce::Colour ACCENT{0xff5b8def};
	inline const juce::Colour ACCENT_TEXT{0xffffffff};
	inline const juce::Colour RECORD{0xffe5484d};
	inline const juce::Colour MUTE{0xff5b8def};
	inline const juce::Colour SOLO{0xffd4b84a};
	inline const juce::Colour BYPASS{0xffd9813b};
	inline const juce::Colour GRID_BAR{0xff3a3d43};
	inline const juce::Colour GRID_BEAT{0xff2a2c31};
	inline const juce::Colour LANE_SELECTED{0xff24262b};
	inline const juce::Colour BACKDROP{0x99000000};
	inline const juce::Colour GROOVE{0xff141518};
	inline const juce::Colour FADER_CAP{0xffc9ccd2};
	inline const juce::Colour FADER_CAP_LINE{0xff4a4d53};
}

class ThemeLookAndFeel : public juce::LookAndFeel_V4
{
public:
	static constexpr int SLIDER_END_MARGIN = 5;

	ThemeLookAndFeel();

	int getSliderThumbRadius(juce::Slider& slider) override;

	void drawLinearSlider(juce::Graphics& graphics, int x, int y, int width, int height, float slider_position, float minimum_position, float maximum_position, juce::Slider::SliderStyle style, juce::Slider& slider) override;
	void drawRotarySlider(juce::Graphics& graphics, int x, int y, int width, int height, float position, float start_angle, float end_angle, juce::Slider& slider) override;
};
