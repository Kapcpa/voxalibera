#include "ui/tracks/new_track_dialog.h"

#include "ui/settings/settings_layout.h"

namespace
{
	constexpr int WIDTH = 480;
	constexpr int HEIGHT = 200;
	constexpr int FALLBACK_INPUT_COUNT = 2;
	constexpr int MONO_ID = 1;
	constexpr int STEREO_ID = 2;
}

NewTrackDialog::NewTrackDialog(AppContext& context) : context(context)
{
	rows[0].label.setText("Name", juce::dontSendNotification);
	rows[0].field = &name;
	rows[1].label.setText("Channels", juce::dontSendNotification);
	rows[1].field = &channels;
	rows[2].label.setText("Input", juce::dontSendNotification);
	rows[2].field = &input;

	channels.addItem("Mono", MONO_ID);
	channels.addItem("Stereo", STEREO_ID);
	channels.onChange = [this] { list_inputs(); };

	name.onReturnKey = [this] { create(); };
	name.onEscapeKey = [this] { on_finished(); };
	cancel_button.onClick = [this] { on_finished(); };
	create_button.onClick = [this] { create(); };

	for (auto& row : rows) {
		addAndMakeVisible(row.label);
		addAndMakeVisible(row.field);
	}

	addAndMakeVisible(cancel_button);
	addAndMakeVisible(create_button);
	setSize(WIDTH, HEIGHT);
}

void NewTrackDialog::prepare()
{
	name.setText(context.tracks.next_default_name(), juce::dontSendNotification);
	name.selectAll();
	channels.setSelectedId(MONO_ID, juce::dontSendNotification);
	list_inputs();
	name.grabKeyboardFocus();
}

juce::StringArray NewTrackDialog::device_input_names() const
{
	if (auto* device = context.engine.device_manager.getCurrentAudioDevice())
		return device->getInputChannelNames();

	juce::StringArray names;

	for (int channel = 1; channel <= FALLBACK_INPUT_COUNT; ++channel)
		names.add("Input " + juce::String(channel));

	return names;
}

TrackChannels NewTrackDialog::chosen_channels() const
{
	return channels.getSelectedId() == STEREO_ID ? TrackChannels::stereo : TrackChannels::mono;
}

void NewTrackDialog::list_inputs()
{
	const auto names = device_input_names();
	const auto track_channels = chosen_channels();
	const auto step = channel_count(track_channels);

	input.clear(juce::dontSendNotification);
	input_choices.clear();

	for (int first = 0; first + step <= names.size(); first += step) {
		const TrackInput choice{first};
		input_choices.push_back(choice);
		input.addItem(describe_input(choice, track_channels), (int) input_choices.size());
	}

	input_choices.push_back({TrackInput::NONE});
	input.addItem(describe_input({TrackInput::NONE}, track_channels), (int) input_choices.size());
	input.setSelectedItemIndex(0, juce::dontSendNotification);
}

void NewTrackDialog::create()
{
	const auto chosen = input.getSelectedItemIndex();
	context.tracks.add({name.getText().trim(), chosen_channels(), input_choices[(size_t) std::max(0, chosen)]});
	on_finished();
}

void NewTrackDialog::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds().reduced(PAGE_PADDING);

	for (auto& row : rows) {
		auto line = area.removeFromTop(ROW_HEIGHT);
		row.label.setBounds(line.removeFromLeft(LABEL_WIDTH));
		row.field->setBounds(line);
		area.removeFromTop(ROW_GAP);
	}

	auto buttons = area.removeFromBottom(ROW_HEIGHT);
	create_button.setBounds(buttons.removeFromRight(BUTTON_WIDTH));
	buttons.removeFromRight(ROW_GAP);
	cancel_button.setBounds(buttons.removeFromRight(BUTTON_WIDTH));
}
