#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

class TrackHeader : public juce::Component
{
public:
	static constexpr int WIDTH = 240;

	TrackHeader(AppContext& context, TrackId id);

	void show(const Track& track, bool selected);
	void paint(juce::Graphics& graphics) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent& event) override;

private:
	void style_toggle(juce::TextButton& button, juce::Colour on_colour);
	void choose_colour();

	AppContext& context;
	TrackId id;
	juce::Colour colour;
	bool selected = false;
	juce::Label name;
	juce::Label details;
	juce::TextButton arm_button{"R"};
	juce::TextButton mute_button{"M"};
	juce::TextButton solo_button{"S"};
	juce::Slider volume{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
};
