#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

class TransportBar : public juce::Component
{
public:
	explicit TransportBar(AppContext& context);

	void update();
	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	void show_playing(bool playing);
	void show_recording(bool recording);
	void show_position();
	void show_project_values();
	void show_rate_warning();
	void choose_signature();

	AppContext& context;
	juce::ShapeButton start_button;
	juce::ShapeButton play_button;
	juce::ShapeButton record_button;
	juce::Label musical_position;
	juce::Label clock_position;
	juce::Slider tempo_field{juce::Slider::LinearBarVertical, juce::Slider::TextBoxBelow};
	juce::ComboBox signature_box;
	juce::TextButton metronome_button{"Click"};
	juce::Label rate_warning;
	bool shown_playing = true;
	bool shown_recording = true;
	TimeSignature shown_signature{0, 0};
};
