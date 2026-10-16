#include "ui/tracks/track_header.h"

#include "core/project/model.h"
#include "ui/theme.h"

namespace
{
	constexpr int COLOUR_STRIP_WIDTH = 6;
	constexpr int PADDING = 8;
	constexpr int ROW_HEIGHT = 22;
	constexpr int TOGGLE_SIZE = 22;
	constexpr int PLUGINS_BUTTON_WIDTH = 28;
	constexpr int TOGGLE_GAP = 3;
	constexpr int DETAILS_WIDTH = 82;
	constexpr int VOLUME_TEXT_WIDTH = 46;
	constexpr float NAME_FONT_SIZE = 14.0f;
	constexpr float DETAILS_FONT_SIZE = 12.0f;
	constexpr double MIN_VOLUME_DB = -60.0;
	constexpr double MAX_VOLUME_DB = 6.0;
	constexpr double VOLUME_STEP_DB = 0.1;
	constexpr double VOLUME_SKEW_CENTRE_DB = -12.0;

}

TrackHeader::TrackHeader(AppContext& context, TrackId id) : context(context), id(id)
{
	name.setFont(juce::FontOptions(NAME_FONT_SIZE, juce::Font::bold));
	name.setEditable(false, true, false);
	name.setInterceptsMouseClicks(false, false);
	name.onTextChange = [this] { this->context.tracks.rename(this->id, name.getText()); };

	details.setFont(juce::FontOptions(DETAILS_FONT_SIZE));
	details.setColour(juce::Label::textColourId, theme::DIM_TEXT);
	details.setInterceptsMouseClicks(false, false);

	style_toggle(arm_button, theme::RECORD);
	style_toggle(mute_button, theme::MUTE);
	style_toggle(solo_button, theme::SOLO);
	arm_button.onClick = [this] { this->context.tracks.toggle(this->id, &Track::armed, "Arm Track"); };
	mute_button.onClick = [this] { this->context.tracks.toggle(this->id, &Track::mute, "Mute Track"); };
	solo_button.onClick = [this] { this->context.tracks.toggle(this->id, &Track::solo, "Solo Track"); };
	plugins_button.onClick = [this] {
		if (on_show_plugins)
			on_show_plugins(this->id);
	};

	volume.setRange(MIN_VOLUME_DB, MAX_VOLUME_DB, VOLUME_STEP_DB);
	volume.setSkewFactorFromMidPoint(VOLUME_SKEW_CENTRE_DB);
	volume.setDoubleClickReturnValue(true, 0.0);
	volume.setScrollWheelEnabled(false);
	volume.setTextValueSuffix(" dB");
	volume.setNumDecimalPlacesToDisplay(1);
	volume.setTextBoxStyle(juce::Slider::TextBoxRight, false, VOLUME_TEXT_WIDTH, ROW_HEIGHT);
	volume.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
	volume.onValueChange = [this] { this->context.tracks.set_volume(this->id, (float) volume.getValue()); };

	for (auto* child : std::initializer_list<juce::Component*>{&name, &details, &plugins_button, &arm_button, &mute_button, &solo_button, &volume})
		addAndMakeVisible(child);
}

void TrackHeader::style_toggle(juce::TextButton& button, juce::Colour on_colour)
{
	button.setColour(juce::TextButton::buttonOnColourId, on_colour);
	button.setColour(juce::TextButton::textColourOnId, theme::WINDOW_BACKGROUND);
	button.setWantsKeyboardFocus(false);
}

void TrackHeader::show(const Track& track, bool is_selected)
{
	colour = juce::Colour(track.colour);
	selected = is_selected;

	if (!name.isBeingEdited())
		name.setText(track.name, juce::dontSendNotification);

	details.setText(describe_channels(track.channels) + " · " + describe_input(track.input, track.channels), juce::dontSendNotification);
	arm_button.setToggleState(track.armed, juce::dontSendNotification);
	mute_button.setToggleState(track.mute, juce::dontSendNotification);
	solo_button.setToggleState(track.solo, juce::dontSendNotification);
	plugins_button.show(track);

	if (!volume.isMouseButtonDown())
		volume.setValue(track.volume_db, juce::dontSendNotification);

	repaint();
}

void TrackHeader::paint(juce::Graphics& graphics)
{
	graphics.fillAll(selected ? theme::WIDGET_BACKGROUND : theme::PANEL_BACKGROUND);
	graphics.setColour(colour);
	graphics.fillRect(getLocalBounds().removeFromLeft(COLOUR_STRIP_WIDTH));
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(getHeight() - 1, 0.0f, (float) getWidth());
}

void TrackHeader::resized()
{
	auto area = getLocalBounds().withTrimmedLeft(COLOUR_STRIP_WIDTH).reduced(PADDING, PADDING / 2);

	auto top = area.removeFromTop(ROW_HEIGHT);

	for (auto* button : {&solo_button, &mute_button, &arm_button}) {
		button->setBounds(top.removeFromRight(TOGGLE_SIZE));
		top.removeFromRight(TOGGLE_GAP);
	}

	plugins_button.setBounds(top.removeFromRight(PLUGINS_BUTTON_WIDTH));
	top.removeFromRight(TOGGLE_GAP);

	name.setBounds(top);
	area.removeFromTop(TOGGLE_GAP);

	auto bottom = area.removeFromTop(ROW_HEIGHT);
	details.setBounds(bottom.removeFromLeft(DETAILS_WIDTH));
	volume.setBounds(bottom);
}

void TrackHeader::mouseDown(const juce::MouseEvent& event)
{
	if (event.x < COLOUR_STRIP_WIDTH + PADDING) {
		choose_colour();
		return;
	}

	if (event.mods.isShiftDown())
		context.selection.toggle(id);
	else
		context.selection.select_only(id);

	if (event.getNumberOfClicks() != 2)
		return;

	if (name.getBounds().contains(event.getPosition()))
		name.showEditor();
	else if (details.getBounds().contains(event.getPosition()) && on_edit)
		on_edit(id);
}

void TrackHeader::choose_colour()
{
	juce::PopupMenu menu;

	for (size_t position = 0; position < TRACK_COLOURS.size(); ++position)
		menu.addColouredItem((int) position + 1, TRACK_COLOURS[position].name, juce::Colour(TRACK_COLOURS[position].argb));

	menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [header = juce::Component::SafePointer<TrackHeader>(this)](int choice) {
		if (header != nullptr && choice > 0)
			header->context.tracks.set_colour(header->id, TRACK_COLOURS[(size_t) choice - 1].argb);
	});
}
