#include "ui/mixer/mixer_strip.h"

#include "engine/levels.h"
#include "ui/theme.h"
#include "ui/theme_look_and_feel.h"

#include <cmath>

namespace
{
	constexpr int PADDING = 6;
	constexpr int COLOUR_BAR_HEIGHT = 3;
	constexpr int PAN_SIZE = 34;
	constexpr int VALUE_HEIGHT = 16;
	constexpr int BUTTON_HEIGHT = 20;
	constexpr int BUTTON_GAP = 4;
	constexpr int NAME_PLATE_HEIGHT = 22;
	constexpr int FADER_WIDTH = 26;
	constexpr int METER_WIDTH = 9;
	constexpr int FADER_METER_GAP = 5;
	constexpr int GAP = 4;
	constexpr float NAME_FONT_SIZE = 12.0f;
	constexpr float VALUE_FONT_SIZE = 11.0f;
	constexpr float NAME_PLATE_ALPHA = 0.55f;
	constexpr double MIN_VOLUME_DB = -60.0;
	constexpr double MAX_VOLUME_DB = 6.0;
	constexpr double VOLUME_STEP_DB = 0.1;
	constexpr double VOLUME_SKEW_CENTRE_DB = -12.0;
	constexpr double PAN_STEP = 0.01;
	constexpr int PAN_PERCENT = 100;

	juce::String describe_pan(double pan)
	{
		const auto percent = (int) std::lround(pan * PAN_PERCENT);

		if (percent == 0)
			return "C";

		return (percent < 0 ? "L" : "R") + juce::String(std::abs(percent));
	}

	juce::String describe_volume(float volume_db)
	{
		return volume_db <= SILENT_DB ? juce::String("-inf") : juce::String(volume_db, 1);
	}

	void style_value(juce::Label& label)
	{
		label.setFont(juce::FontOptions(VALUE_FONT_SIZE));
		label.setJustificationType(juce::Justification::centred);
		label.setColour(juce::Label::textColourId, theme::DIM_TEXT.brighter(0.5f));
	}
}

MixerStrip::MixerStrip(AppContext& context, TrackId id) : context(context), id(id)
{
	name.setFont(juce::FontOptions(NAME_FONT_SIZE, juce::Font::bold));
	name.setJustificationType(juce::Justification::centred);
	name.setInterceptsMouseClicks(false, false);

	pan.setRange(-1.0, 1.0, PAN_STEP);
	pan.setDoubleClickReturnValue(true, 0.0);
	pan.setScrollWheelEnabled(false);
	pan.onValueChange = [this] {
		pan_value.setText(describe_pan(pan.getValue()), juce::dontSendNotification);
		this->context.tracks.set_pan(this->id, (float) pan.getValue());
	};
	style_value(pan_value);
	pan_value.setInterceptsMouseClicks(false, false);

	fader.setRange(MIN_VOLUME_DB, MAX_VOLUME_DB, VOLUME_STEP_DB);
	fader.setSkewFactorFromMidPoint(VOLUME_SKEW_CENTRE_DB);
	fader.setDoubleClickReturnValue(true, 0.0);
	fader.setScrollWheelEnabled(false);
	fader.onValueChange = [this] {
		show_volume((float) fader.getValue());
		set_volume((float) fader.getValue());
	};

	style_value(volume_value);
	volume_value.setEditable(false, true, false);
	volume_value.onTextChange = [this] { set_volume((float) volume_value.getText().getDoubleValue()); };

	style_toggle(mute_button, theme::MUTE);
	style_toggle(solo_button, theme::SOLO);
	mute_button.onClick = [this] { this->context.tracks.toggle(this->id, &Track::mute, "Mute Track"); };
	solo_button.onClick = [this] { this->context.tracks.toggle(this->id, &Track::solo, "Solo Track"); };

	for (auto* child : std::initializer_list<juce::Component*>{&name, &volume_value, &fader, &meter_view})
		addAndMakeVisible(child);

	if (!is_master())
		for (auto* child : std::initializer_list<juce::Component*>{&pan, &pan_value, &mute_button, &solo_button})
			addAndMakeVisible(child);
}

void MixerStrip::style_toggle(juce::TextButton& button, juce::Colour on_colour)
{
	button.setColour(juce::TextButton::buttonOnColourId, on_colour);
	button.setColour(juce::TextButton::textColourOnId, theme::WINDOW_BACKGROUND);
	button.setWantsKeyboardFocus(false);
}

void MixerStrip::set_volume(float volume_db)
{
	if (is_master())
		context.set_master_volume(volume_db);
	else
		context.tracks.set_volume(id, volume_db);
}

void MixerStrip::show_volume(float volume_db)
{
	if (!volume_value.isBeingEdited())
		volume_value.setText(describe_volume(volume_db), juce::dontSendNotification);
}

void MixerStrip::show_track(const Track& track, bool is_selected, LevelMeter* meter)
{
	colour = juce::Colour(track.colour);
	selected = is_selected;
	name.setText(track.name, juce::dontSendNotification);
	mute_button.setToggleState(track.mute, juce::dontSendNotification);
	solo_button.setToggleState(track.solo, juce::dontSendNotification);

	if (!pan.isMouseButtonDown())
		pan.setValue(track.pan, juce::dontSendNotification);

	if (!fader.isMouseButtonDown())
		fader.setValue(track.volume_db, juce::dontSendNotification);

	pan_value.setText(describe_pan(track.pan), juce::dontSendNotification);
	show_volume(track.volume_db);
	meter_view.show(meter);
	repaint();
}

void MixerStrip::show_master(float volume_db, LevelMeter* meter)
{
	colour = theme::DIM_TEXT;
	name.setText("Master", juce::dontSendNotification);

	if (!fader.isMouseButtonDown())
		fader.setValue(volume_db, juce::dontSendNotification);

	show_volume(volume_db);
	meter_view.show(meter);
	repaint();
}

void MixerStrip::update_meter()
{
	meter_view.update();
}

void MixerStrip::paint(juce::Graphics& graphics)
{
	graphics.fillAll(selected ? theme::WIDGET_BACKGROUND : theme::PANEL_BACKGROUND);

	graphics.setColour(colour);
	graphics.fillRect(getLocalBounds().removeFromTop(COLOUR_BAR_HEIGHT));

	graphics.setColour(colour.withAlpha(NAME_PLATE_ALPHA));
	graphics.fillRect(name_plate);

	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawVerticalLine(getWidth() - 1, 0.0f, (float) getHeight());
}

void MixerStrip::resized()
{
	auto area = getLocalBounds().withTrimmedTop(COLOUR_BAR_HEIGHT);

	name_plate = area.removeFromBottom(NAME_PLATE_HEIGHT);
	name.setBounds(name_plate);
	area.reduce(PADDING, PADDING);

	auto buttons = area.removeFromBottom(BUTTON_HEIGHT);
	const auto button_width = (buttons.getWidth() - BUTTON_GAP) / 2;
	mute_button.setBounds(buttons.removeFromLeft(button_width));
	solo_button.setBounds(buttons.removeFromRight(button_width));
	area.removeFromBottom(GAP);

	pan.setBounds(area.removeFromTop(PAN_SIZE).withSizeKeepingCentre(PAN_SIZE, PAN_SIZE));
	pan_value.setBounds(area.removeFromTop(VALUE_HEIGHT));
	area.removeFromTop(GAP);
	volume_value.setBounds(area.removeFromTop(VALUE_HEIGHT));

	auto fader_and_meter = area.withSizeKeepingCentre(FADER_WIDTH + FADER_METER_GAP + METER_WIDTH, area.getHeight());
	fader.setBounds(fader_and_meter.removeFromLeft(FADER_WIDTH));
	fader_and_meter.removeFromLeft(FADER_METER_GAP);
	meter_view.setBounds(fader_and_meter.removeFromLeft(METER_WIDTH).reduced(0, ThemeLookAndFeel::SLIDER_END_MARGIN));
}

void MixerStrip::mouseDown(const juce::MouseEvent& event)
{
	if (is_master())
		return;

	if (event.mods.isShiftDown())
		context.selection.toggle(id);
	else
		context.selection.select_only(id);
}
