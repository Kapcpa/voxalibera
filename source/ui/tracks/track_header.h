#pragma once

#include "core/app_context.h"
#include "ui/parts/plugins_button.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class TrackHeader : public juce::Component
{
public:
	static constexpr int WIDTH = 240;

	TrackHeader(AppContext& context, TrackId id);

	void show(const Track& track, bool selected);
	void paint(juce::Graphics& graphics) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseMove(const juce::MouseEvent& event) override;
	void mouseExit(const juce::MouseEvent& event) override;

	std::function<void(TrackId)> on_show_plugins;
	std::function<void(TrackId)> on_edit;

private:
	void style_toggle(juce::TextButton& button, juce::Colour on_colour);
	void choose_colour();
	juce::Rectangle<int> expander_area() const;
	void paint_expander(juce::Graphics& graphics);

	AppContext& context;
	TrackId id;
	juce::Colour colour;
	bool selected = false;
	bool expandable = false;
	bool expanded = false;
	bool expander_hovered = false;
	juce::Label name;
	juce::Label details;
	PluginsButton plugins_button;
	juce::TextButton arm_button{"R"};
	juce::TextButton mute_button{"M"};
	juce::TextButton solo_button{"S"};
	juce::Slider volume{juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight};
	juce::Label takes_count;
	juce::TextButton keep_comp_button{"Keep Comp"};
};
