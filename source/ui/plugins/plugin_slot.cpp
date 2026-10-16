#include "ui/plugins/plugin_slot.h"

#include "ui/theme.h"

namespace
{
	constexpr auto BYPASS_TEXT = "B";
	constexpr auto REMOVE_TEXT = "\xc3\x97";
	constexpr auto MISSING_TEXT = "not available";
	constexpr float CORNER_RADIUS = 4.0f;
	constexpr float BORDER_WIDTH = 1.0f;
	constexpr float SELECTED_BORDER_WIDTH = 2.0f;
	constexpr float BYPASSED_DARKNESS = 0.25f;
	constexpr float ACTIVE_BRIGHTNESS = 0.05f;
	constexpr float NAME_FONT_SIZE = 13.0f;
	constexpr float FORMAT_FONT_SIZE = 10.0f;
	constexpr int MARGIN = 2;
	constexpr int TEXT_MARGIN = 8;
	constexpr int NAME_TOP = 22;
	constexpr int NAME_HEIGHT = 36;
	constexpr int NAME_LINES = 2;
	constexpr int FORMAT_HEIGHT = 16;
	constexpr int FORMAT_VERTICAL_MARGIN = 6;
	constexpr int BUTTON_AREA_MARGIN = 4;
	constexpr int BUTTON_SIZE = 18;
	constexpr int BUTTON_GAP = 2;
	constexpr int DRAG_THRESHOLD = 4;
}

PluginSlot::PluginSlot(AppContext& context, const Plugin& plugin) : context(context), plugin_data(plugin)
{
	bypass_button.setButtonText(BYPASS_TEXT);
	bypass_button.onClick = [this] { this->context.tracks.toggle_plugin_bypass(plugin_data.id); };
	addAndMakeVisible(bypass_button);

	remove_button.setButtonText(juce::CharPointer_UTF8(REMOVE_TEXT));
	remove_button.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
	remove_button.onClick = [this] { this->context.tracks.remove_plugin(plugin_data.id); };
	addAndMakeVisible(remove_button);

	show_bypass();
}

void PluginSlot::show(const Plugin& plugin, bool selected, bool missing, bool chain_bypassed)
{
	if (plugin.name == plugin_data.name && plugin.bypass == plugin_data.bypass && selected == is_selected && missing == is_missing && chain_bypassed == is_chain_bypassed)
		return;

	plugin_data = plugin;
	is_selected = selected;
	is_missing = missing;
	is_chain_bypassed = chain_bypassed;
	show_bypass();
	repaint();
}

void PluginSlot::show_bypass()
{
	bypass_button.setColour(juce::TextButton::buttonColourId, plugin_data.bypass ? theme::BYPASS : juce::Colours::transparentBlack);
}

void PluginSlot::paint(juce::Graphics& graphics)
{
	const auto area = getLocalBounds().reduced(MARGIN).toFloat();
	const auto text_area = area.toNearestInt().reduced(TEXT_MARGIN, 0);
	const auto dimmed = plugin_data.bypass || is_missing || is_chain_bypassed;

	graphics.setColour(dimmed ? theme::PANEL_BACKGROUND.darker(BYPASSED_DARKNESS) : theme::PANEL_BACKGROUND.brighter(ACTIVE_BRIGHTNESS));
	graphics.fillRoundedRectangle(area, CORNER_RADIUS);

	graphics.setColour(is_selected ? theme::ACCENT : theme::PANEL_BORDER);
	graphics.drawRoundedRectangle(area, CORNER_RADIUS, is_selected ? SELECTED_BORDER_WIDTH : BORDER_WIDTH);

	graphics.setColour(dimmed ? theme::DIM_TEXT : theme::TEXT);
	graphics.setFont(juce::FontOptions(NAME_FONT_SIZE, juce::Font::bold));
	graphics.drawFittedText(plugin_data.name, text_area.withTrimmedTop(NAME_TOP).withHeight(NAME_HEIGHT), juce::Justification::topLeft, NAME_LINES);

	graphics.setColour(is_missing ? theme::RECORD : theme::DIM_TEXT);
	graphics.setFont(juce::FontOptions(FORMAT_FONT_SIZE));
	graphics.drawText(is_missing ? MISSING_TEXT : plugin_data.format, text_area.reduced(0, FORMAT_VERTICAL_MARGIN).removeFromBottom(FORMAT_HEIGHT), juce::Justification::bottomLeft, false);
}

void PluginSlot::resized()
{
	auto buttons = getLocalBounds().reduced(BUTTON_AREA_MARGIN).removeFromTop(BUTTON_SIZE);
	remove_button.setBounds(buttons.removeFromRight(BUTTON_SIZE));
	buttons.removeFromRight(BUTTON_GAP);
	bypass_button.setBounds(buttons.removeFromRight(BUTTON_SIZE));
}

void PluginSlot::mouseDrag(const juce::MouseEvent& event)
{
	if (!dragging && event.getDistanceFromDragStart() < DRAG_THRESHOLD)
		return;

	dragging = true;

	if (on_drag != nullptr)
		on_drag(event);
}

void PluginSlot::mouseUp(const juce::MouseEvent& event)
{
	if (dragging) {
		dragging = false;

		if (on_drop != nullptr)
			on_drop(event);

		return;
	}

	if (!event.mods.isPopupMenu() && on_click != nullptr)
		on_click();
}
