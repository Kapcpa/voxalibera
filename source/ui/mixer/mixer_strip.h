#pragma once

#include "core/app_context.h"
#include "ui/parts/plugins_button.h"
#include "ui/parts/meter_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class MixerStrip : public juce::Component
{
public:
	static constexpr int WIDTH = 76;

	MixerStrip(AppContext& context, TrackId id);

	bool is_master() const { return !id.is_valid(); }
	void show_track(const Track& track, bool selected, LevelMeter* meter);
	void show_master(float volume_db, LevelMeter* meter);
	void update_meter();

	void paint(juce::Graphics& graphics) override;
	void resized() override;
	void mouseDown(const juce::MouseEvent& event) override;

	std::function<void(TrackId)> on_show_plugins;

private:
	void set_volume(float volume_db);
	void show_volume(float volume_db);
	void style_toggle(juce::TextButton& button, juce::Colour on_colour);

	AppContext& context;
	TrackId id;
	juce::Colour colour;
	bool selected = false;
	juce::Rectangle<int> name_plate;
	juce::Label name;
	juce::Slider pan{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox};
	juce::Label pan_value;
	juce::Label volume_value;
	juce::Slider fader{juce::Slider::LinearVertical, juce::Slider::NoTextBox};
	MeterView meter_view{MeterView::Orientation::vertical};
	juce::TextButton mute_button{"M"};
	juce::TextButton solo_button{"S"};
	PluginsButton plugins_button;
};
