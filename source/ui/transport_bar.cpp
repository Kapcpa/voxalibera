#include "ui/transport_bar.h"

#include "ui/theme.h"

#include <array>
#include <cmath>

namespace
{
	constexpr int PADDING = 8;
	constexpr int GAP = 6;
	constexpr int GROUP_GAP = 20;
	constexpr int BUTTON_SIZE = 32;
	constexpr int ICON_MARGIN = 9;
	constexpr int MUSICAL_WIDTH = 120;
	constexpr int CLOCK_WIDTH = 100;
	constexpr int TEMPO_WIDTH = 96;
	constexpr int SIGNATURE_WIDTH = 72;
	constexpr int METRONOME_WIDTH = 64;
	constexpr int COUNT_IN_WIDTH = 80;
	constexpr int CONTROL_HEIGHT = 28;
	constexpr float POSITION_FONT_SIZE = 22.0f;
	constexpr float CLOCK_FONT_SIZE = 14.0f;
	constexpr double TEMPO_STEP = 0.1;
	constexpr int TEMPO_DECIMALS = 1;
	constexpr int TEMPO_DRAG_SENSITIVITY = 800;
	constexpr float RECORDING_OUTLINE = 1.5f;
	constexpr int WARNING_MIN_WIDTH = 200;

	const std::array<TimeSignature, 7> SIGNATURES{{{2, 4}, {3, 4}, {4, 4}, {5, 4}, {6, 8}, {7, 8}, {12, 8}}};

	juce::Path return_to_start_icon()
	{
		juce::Path path;
		path.addRectangle(0.0f, 0.0f, 2.0f, 10.0f);
		path.addTriangle(10.0f, 0.0f, 10.0f, 10.0f, 2.5f, 5.0f);
		return path;
	}

	juce::Path play_icon()
	{
		juce::Path path;
		path.addTriangle(0.0f, 0.0f, 0.0f, 10.0f, 8.66f, 5.0f);
		return path;
	}

	juce::Path stop_icon()
	{
		juce::Path path;
		path.addRectangle(0.0f, 0.0f, 10.0f, 10.0f);
		return path;
	}

	juce::Path record_icon()
	{
		juce::Path path;
		path.addEllipse(0.0f, 0.0f, 10.0f, 10.0f);
		return path;
	}

	void style_icon_button(juce::ShapeButton& button, const juce::Path& icon)
	{
		button.setShape(icon, false, true, false);
		button.setBorderSize(juce::BorderSize<int>(ICON_MARGIN));
		button.setWantsKeyboardFocus(false);
	}
}

TransportBar::TransportBar(AppContext& context) : context(context), start_button("Return to start", theme::TEXT, theme::ACCENT_TEXT, theme::ACCENT), play_button("Play", theme::TEXT, theme::ACCENT_TEXT, theme::ACCENT), record_button("Record", theme::RECORD, theme::RECORD, theme::RECORD)
{
	style_icon_button(start_button, return_to_start_icon());
	style_icon_button(play_button, play_icon());
	style_icon_button(record_button, record_icon());

	start_button.onClick = [this] { this->context.return_to_start(); };
	play_button.onClick = [this] { this->context.toggle_playback(); };
	record_button.onClick = [this] { this->context.toggle_recording(); };

	musical_position.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), POSITION_FONT_SIZE, juce::Font::bold));
	clock_position.setFont(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), CLOCK_FONT_SIZE, juce::Font::plain));
	clock_position.setColour(juce::Label::textColourId, theme::DIM_TEXT);

	tempo_field.setRange(Project::MIN_TEMPO, Project::MAX_TEMPO, TEMPO_STEP);
	tempo_field.setNumDecimalPlacesToDisplay(TEMPO_DECIMALS);
	tempo_field.setTextValueSuffix(" bpm");
	tempo_field.setMouseDragSensitivity(TEMPO_DRAG_SENSITIVITY);
	tempo_field.setColour(juce::Slider::trackColourId, juce::Colours::transparentBlack);
	tempo_field.setWantsKeyboardFocus(false);
	tempo_field.setColour(juce::Slider::backgroundColourId, theme::WIDGET_BACKGROUND);
	tempo_field.onValueChange = [this] { this->context.set_tempo(tempo_field.getValue()); };

	for (size_t position = 0; position < SIGNATURES.size(); ++position)
		signature_box.addItem(format_time_signature(SIGNATURES[position]), (int) position + 1);

	signature_box.setEditableText(true);
	signature_box.onChange = [this] { choose_signature(); };
	signature_box.setWantsKeyboardFocus(false);

	metronome_button.setColour(juce::TextButton::buttonOnColourId, theme::ACCENT);
	metronome_button.setWantsKeyboardFocus(false);
	metronome_button.onClick = [this] { this->context.toggle_metronome(); };
	metronome_button.setTooltip("Metronome (K)");

	count_in_button.setColour(juce::TextButton::buttonOnColourId, theme::ACCENT);
	count_in_button.setWantsKeyboardFocus(false);
	count_in_button.onClick = [this] { this->context.toggle_count_in(); };
	count_in_button.setTooltip("Count in one bar before recording (Shift+K)");

	rate_warning.setColour(juce::Label::textColourId, theme::RECORD);
	rate_warning.setJustificationType(juce::Justification::centredRight);
	rate_warning.setMinimumHorizontalScale(1.0f);

	for (auto* child : std::initializer_list<juce::Component*>{&start_button, &play_button, &record_button, &musical_position, &clock_position, &tempo_field, &signature_box, &metronome_button, &count_in_button, &rate_warning})
		addAndMakeVisible(child);

	update();
}

void TransportBar::update()
{
	show_playing(context.engine.transport.is_playing());
	show_recording(context.recorder.is_recording());
	show_position();
	show_project_values();
	show_rate_warning();
}

void TransportBar::show_rate_warning()
{
	const auto file_rate = context.publisher.mismatched_file_rate();
	const auto device_rate = context.engine.sample_rate.load();
	const auto text = file_rate > 0.0 && device_rate > 0.0 ? "Clips at " + juce::String(file_rate, 0) + " Hz are silent: too far from the device rate of " + juce::String(device_rate, 0) + " Hz" : juce::String();

	if (rate_warning.getText() == text)
		return;

	rate_warning.setText(text, juce::dontSendNotification);
	rate_warning.setTooltip(text.isEmpty() ? juce::String() : "Set the sample rate to " + juce::String(file_rate, 0) + " Hz in Settings > Audio");
}

void TransportBar::show_playing(bool playing)
{
	if (playing == shown_playing)
		return;

	shown_playing = playing;
	play_button.setShape(playing ? stop_icon() : play_icon(), false, true, false);
	play_button.setTooltip(playing ? "Stop (Space)" : "Play (Space)");
}

void TransportBar::show_recording(bool recording)
{
	if (recording == shown_recording)
		return;

	shown_recording = recording;
	record_button.setColours(recording ? theme::RECORD : theme::RECORD.withAlpha(0.55f), theme::RECORD, theme::RECORD);
	record_button.setOutline(recording ? theme::TEXT : juce::Colours::transparentBlack, RECORDING_OUTLINE);
	musical_position.setColour(juce::Label::textColourId, recording ? theme::RECORD : theme::TEXT);
	record_button.setTooltip(recording ? "Stop recording (Ctrl+R)" : "Record (Ctrl+R)");
}

void TransportBar::show_position()
{
	const auto sample = context.engine.transport.position();
	const auto rate = context.engine.sample_rate.load();

	if (context.recorder.is_counting_in() && rate > 0.0) {
		const auto beats_left = (int) std::ceil((double) (context.engine.transport.record_from() - sample) / samples_per_beat(context.project.tempo, rate));
		musical_position.setText(juce::String(beats_left), juce::dontSendNotification);
		clock_position.setText("Count-in", juce::dontSendNotification);
		return;
	}

	const auto musical = rate > 0.0 ? to_musical_position(sample, context.project.tempo, rate) : MusicalPosition{};

	musical_position.setText(format_musical_position(musical), juce::dontSendNotification);
	clock_position.setText(format_clock(sample, rate), juce::dontSendNotification);
}

void TransportBar::show_project_values()
{
	const auto& tempo = context.project.tempo;

	if (!tempo_field.isMouseButtonDown())
		tempo_field.setValue(tempo.quarter_notes_per_minute, juce::dontSendNotification);

	if (tempo.signature != shown_signature) {
		shown_signature = tempo.signature;
		signature_box.setText(format_time_signature(tempo.signature), juce::dontSendNotification);
	}

	metronome_button.setToggleState(context.project.metronome, juce::dontSendNotification);
	count_in_button.setToggleState(context.project.count_in, juce::dontSendNotification);
}

void TransportBar::choose_signature()
{
	if (const auto signature = parse_time_signature(signature_box.getText()))
		context.set_time_signature(*signature);

	signature_box.setText(format_time_signature(context.project.tempo.signature), juce::dontSendNotification);
	shown_signature = context.project.tempo.signature;
}

void TransportBar::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::PANEL_BACKGROUND);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(getHeight() - 1, 0.0f, (float) getWidth());
}

void TransportBar::resized()
{
	auto area = getLocalBounds().reduced(PADDING, 0);
	const auto centred = [&](juce::Rectangle<int> slot, int height) { return slot.withSizeKeepingCentre(slot.getWidth(), height); };

	for (auto* button : {&start_button, &play_button, &record_button}) {
		button->setBounds(centred(area.removeFromLeft(BUTTON_SIZE), BUTTON_SIZE));
		area.removeFromLeft(GAP);
	}

	area.removeFromLeft(GROUP_GAP);
	musical_position.setBounds(area.removeFromLeft(MUSICAL_WIDTH));
	clock_position.setBounds(area.removeFromLeft(CLOCK_WIDTH));

	area.removeFromLeft(GROUP_GAP);
	tempo_field.setBounds(centred(area.removeFromLeft(TEMPO_WIDTH), CONTROL_HEIGHT));
	area.removeFromLeft(GAP);
	signature_box.setBounds(centred(area.removeFromLeft(SIGNATURE_WIDTH), CONTROL_HEIGHT));
	area.removeFromLeft(GROUP_GAP);
	metronome_button.setBounds(centred(area.removeFromLeft(METRONOME_WIDTH), CONTROL_HEIGHT));
	area.removeFromLeft(GAP);
	count_in_button.setBounds(centred(area.removeFromLeft(COUNT_IN_WIDTH), CONTROL_HEIGHT));
	area.removeFromLeft(GROUP_GAP);
	rate_warning.setBounds(area.getWidth() >= WARNING_MIN_WIDTH ? area : juce::Rectangle<int>());
}
