#include "ui/tracks/track_dialog.h"

#include "ui/settings/settings_parts.h"

#include <algorithm>

namespace
{
	constexpr int WIDTH = 480;
	constexpr int HEIGHT = 200;
	constexpr int FALLBACK_INPUT_COUNT = 2;
	constexpr int MONO_ID = 1;
	constexpr int STEREO_ID = 2;
	constexpr auto CREATE_TEXT = "Create";
	constexpr auto APPLY_TEXT = "Apply";

	TrackInput aligned_to(TrackInput input, TrackChannels channels)
	{
		if (!input.is_connected())
			return input;

		const auto step = channel_count(channels);
		return {input.first_channel - input.first_channel % step};
	}
}

TrackDialog::TrackDialog(AppContext& context) : context(context)
{
	rows[0].label.setText("Name", juce::dontSendNotification);
	rows[0].field = &name;
	rows[1].label.setText("Channels", juce::dontSendNotification);
	rows[1].field = &channels;
	rows[2].label.setText("Input", juce::dontSendNotification);
	rows[2].field = &input;

	channels.addItem("Mono", MONO_ID);
	channels.addItem("Stereo", STEREO_ID);
	channels.onChange = [this] { list_inputs(aligned_to(chosen_input(), chosen_channels())); };

	name.onReturnKey = [this] { confirm(); };
	name.onEscapeKey = [this] { on_finished(); };
	cancel_button.onClick = [this] { on_finished(); };
	confirm_button.onClick = [this] { confirm(); };

	for (auto& row : rows) {
		addAndMakeVisible(row.label);
		addAndMakeVisible(row.field);
	}

	addAndMakeVisible(cancel_button);
	addAndMakeVisible(confirm_button);
	setSize(WIDTH, HEIGHT);
}

void TrackDialog::prepare_new()
{
	editing = {};
	confirm_button.setButtonText(CREATE_TEXT);
	show(context.tracks.next_default_name(), TrackChannels::mono, TrackInput{});
}

void TrackDialog::prepare_edit(TrackId id)
{
	const auto* track = context.project.find_track(id);

	if (track == nullptr)
		return;

	editing = id;
	confirm_button.setButtonText(APPLY_TEXT);
	show(track->name, track->channels, track->input);
}

void TrackDialog::show(const juce::String& track_name, TrackChannels track_channels, TrackInput track_input)
{
	name.setText(track_name, juce::dontSendNotification);
	name.selectAll();
	channels.setSelectedId(track_channels == TrackChannels::stereo ? STEREO_ID : MONO_ID, juce::dontSendNotification);
	list_inputs(track_input);
	name.grabKeyboardFocus();
}

juce::StringArray TrackDialog::device_input_names() const
{
	if (auto* device = context.engine.device_manager.getCurrentAudioDevice())
		return device->getInputChannelNames();

	juce::StringArray names;

	for (int channel = 1; channel <= FALLBACK_INPUT_COUNT; ++channel)
		names.add("Input " + juce::String(channel));

	return names;
}

TrackChannels TrackDialog::chosen_channels() const
{
	return channels.getSelectedId() == STEREO_ID ? TrackChannels::stereo : TrackChannels::mono;
}

TrackInput TrackDialog::chosen_input() const
{
	const auto chosen = input.getSelectedItemIndex();
	return chosen < 0 || (size_t) chosen >= input_choices.size() ? TrackInput{} : input_choices[(size_t) chosen];
}

void TrackDialog::list_inputs(TrackInput preferred)
{
	const auto names = device_input_names();
	const auto track_channels = chosen_channels();
	const auto step = channel_count(track_channels);

	input.clear(juce::dontSendNotification);
	input_choices.clear();

	for (int first = 0; first + step <= names.size(); first += step)
		input_choices.push_back({first});

	if (std::find(input_choices.begin(), input_choices.end(), preferred) == input_choices.end() && preferred.is_connected())
		input_choices.push_back(preferred);

	input_choices.push_back({TrackInput::NONE});

	for (size_t index = 0; index < input_choices.size(); ++index)
		input.addItem(describe_input(input_choices[index], track_channels), (int) index + 1);

	const auto selected = std::find(input_choices.begin(), input_choices.end(), preferred);
	input.setSelectedItemIndex(selected == input_choices.end() ? 0 : (int) (selected - input_choices.begin()), juce::dontSendNotification);
}

void TrackDialog::confirm()
{
	const TrackSettings settings{name.getText().trim(), chosen_channels(), chosen_input()};

	if (editing.is_valid())
		context.tracks.change_settings(editing, settings);
	else
		context.tracks.add(settings);

	on_finished();
}

void TrackDialog::resized()
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
	confirm_button.setBounds(buttons.removeFromRight(BUTTON_WIDTH));
	buttons.removeFromRight(ROW_GAP);
	cancel_button.setBounds(buttons.removeFromRight(BUTTON_WIDTH));
}
