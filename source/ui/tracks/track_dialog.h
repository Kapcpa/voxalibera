#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <vector>

class TrackDialog : public juce::Component
{
public:
	explicit TrackDialog(AppContext& context);

	void prepare_new();
	void prepare_edit(TrackId id);
	void resized() override;

	std::function<void()> on_finished;

private:
	struct Row
	{
		juce::Label label;
		juce::Component* field = nullptr;
	};

	void show(const juce::String& track_name, TrackChannels track_channels, TrackInput track_input);
	void list_inputs(TrackInput preferred);
	void confirm();
	juce::StringArray device_input_names() const;
	TrackChannels chosen_channels() const;
	TrackInput chosen_input() const;

	AppContext& context;
	TrackId editing;
	juce::TextEditor name;
	juce::ComboBox channels;
	juce::ComboBox input;
	juce::TextButton cancel_button{"Cancel"};
	juce::TextButton confirm_button;
	std::vector<TrackInput> input_choices;
	std::array<Row, 3> rows;
};
