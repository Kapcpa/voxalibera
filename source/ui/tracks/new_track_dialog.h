#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <functional>
#include <vector>

class NewTrackDialog : public juce::Component
{
public:
	explicit NewTrackDialog(AppContext& context);

	void prepare();
	void resized() override;

	std::function<void()> on_finished;

private:
	struct Row
	{
		juce::Label label;
		juce::Component* field = nullptr;
	};

	void list_inputs();
	void create();
	juce::StringArray device_input_names() const;
	TrackChannels chosen_channels() const;

	AppContext& context;
	juce::TextEditor name;
	juce::ComboBox channels;
	juce::ComboBox input;
	juce::TextButton cancel_button{"Cancel"};
	juce::TextButton create_button{"Create"};
	std::vector<TrackInput> input_choices;
	std::array<Row, 3> rows;
};
