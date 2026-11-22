#include "ui/shortcuts_view.h"

#include "ui/settings/settings_parts.h"
#include "ui/theme.h"

namespace
{
	constexpr int WIDTH = 640;
	constexpr int HEIGHT = 640;
	constexpr int SHORTCUT_ROW_HEIGHT = 22;
	constexpr int TITLE_HEIGHT = 28;
	constexpr int KEY_COLUMN_WIDTH = 210;
	constexpr float ROW_FONT_SIZE = 14.0f;
	constexpr float TITLE_FONT_SIZE = 12.0f;
	constexpr float TITLE_LETTER_SPACING = 0.08f;

	const std::vector<ShortcutGroup> MOUSE_GROUPS{
		{"Tracks area",
		 {
			 {"Wheel", "Scroll tracks up and down"},
			 {"Shift+Wheel", "Scroll sideways"},
			 {"Ctrl+Wheel", "Zoom around the mouse"},
			 {"Click empty space", "Select the track, deselect clips"},
			 {"Drag empty space", "Select the loop range"},
			 {"Click ruler", "Move the playhead, clear the loop range"},
			 {"Drag ruler", "Select the loop range, or drag its edges"},
			 {"Drop audio files", "Import them as clips where they land"},
		 }},
		{"Clips",
		 {
			 {"Click", "Select"},
			 {"Shift+Click", "Add to or remove from the selection"},
			 {"Drag", "Move, snapped to the grid"},
			 {"Ctrl while dragging", "Move or trim without snapping"},
			 {"Drag left or right edge", "Trim"},
			 {"Drag top strip up or down", "Clip gain"},
			 {"Drag fade handle", "Fade in or out (selected clip)"},
		 }},
		{"Take lanes",
		 {
			 {"Click a take", "Play that take there"},
			 {"Drag over a range", "Play that lane over the range"},
			 {"Chevron in track header", "Show or hide the lanes"},
		 }},
		{"Track headers and mixer",
		 {
			 {"Shift+Click", "Add to or remove from the selection"},
			 {"Drag header", "Move the track"},
			 {"Double-click name", "Rename"},
			 {"Double-click channels or input", "Track Settings"},
			 {"Click colour strip", "Change colour"},
			 {"Double-click fader, pan or knob", "Reset"},
		 }},
		{"Effect chain",
		 {
			 {"Click effect, click again", "Select, then open its window"},
			 {"Drag effect onto a wire", "Move it there"},
			 {"Click wire, then +", "Add an effect there"},
		 }},
	};
}

ShortcutsView::ShortcutsView()
{
	viewport.setViewedComponent(&list, false);
	viewport.setScrollBarsShown(true, false);
	addAndMakeVisible(viewport);
	setSize(WIDTH, HEIGHT);
}

void ShortcutsView::show(std::vector<ShortcutGroup> keyboard_groups)
{
	list.groups = std::move(keyboard_groups);
	list.groups.insert(list.groups.end(), MOUSE_GROUPS.begin(), MOUSE_GROUPS.end());
	resized();
	viewport.setViewPosition(0, 0);
}

void ShortcutsView::resized()
{
	viewport.setBounds(getLocalBounds());
	const auto width = viewport.getMaximumVisibleWidth();
	list.setSize(width, list.content_height());
}

int ShortcutsView::List::content_height() const
{
	using namespace settings_layout;
	auto height = PAGE_PADDING;

	for (const auto& group : groups)
		height += TITLE_HEIGHT + (int) group.rows.size() * SHORTCUT_ROW_HEIGHT + SECTION_GAP;

	return height;
}

void ShortcutsView::List::paint(juce::Graphics& graphics)
{
	using namespace settings_layout;
	auto y = PAGE_PADDING;
	const auto text_width = getWidth() - 2 * PAGE_PADDING;

	for (const auto& group : groups) {
		graphics.setColour(theme::DIM_TEXT);
		graphics.setFont(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold).withKerningFactor(TITLE_LETTER_SPACING));
		graphics.drawText(group.title.toUpperCase(), PAGE_PADDING, y, text_width, TITLE_HEIGHT, juce::Justification::centredLeft);
		y += TITLE_HEIGHT;

		graphics.setFont(juce::FontOptions(ROW_FONT_SIZE));

		for (const auto& [keys, action] : group.rows) {
			graphics.setColour(theme::ACCENT);
			graphics.drawText(keys, PAGE_PADDING, y, KEY_COLUMN_WIDTH, SHORTCUT_ROW_HEIGHT, juce::Justification::centredLeft);
			graphics.setColour(theme::TEXT);
			graphics.drawFittedText(action, PAGE_PADDING + KEY_COLUMN_WIDTH, y, text_width - KEY_COLUMN_WIDTH, SHORTCUT_ROW_HEIGHT, juce::Justification::centredLeft, 1);
			y += SHORTCUT_ROW_HEIGHT;
		}

		y += SECTION_GAP;
	}
}
