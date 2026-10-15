#include "ui/plugins/plugin_browser.h"

#include "ui/settings/settings_parts.h"
#include "ui/theme.h"

namespace
{
	constexpr auto SEARCH_HINT = "Search effects";
	constexpr int WIDTH = 520;
	constexpr int HEIGHT = 420;
	constexpr int LIST_ROW_HEIGHT = 28;
	constexpr int TEXT_MARGIN = 8;
	constexpr float NAME_FONT_SIZE = 13.0f;
	constexpr float DETAIL_FONT_SIZE = 11.0f;
	constexpr float NAME_SHARE = 0.6f;
	constexpr int BUTTON_GAP = 6;
}

PluginBrowser::PluginBrowser(AppContext& context) : context(context)
{
	search_field.setTextToShowWhenEmpty(SEARCH_HINT, theme::DIM_TEXT);
	search_field.onTextChange = [this] { search(); };
	search_field.onReturnKey = [this] { choose(list.getSelectedRow()); };
	search_field.onEscapeKey = [this] { on_finished(); };
	search_field.addKeyListener(this);

	list.setRowHeight(LIST_ROW_HEIGHT);
	list.setColour(juce::ListBox::backgroundColourId, theme::PANEL_BACKGROUND);
	status.setColour(juce::Label::textColourId, theme::DIM_TEXT);

	addAndMakeVisible(search_field);
	addAndMakeVisible(list);
	addAndMakeVisible(status);

	cancel_button.onClick = [this] { on_finished(); };
	addAndMakeVisible(cancel_button);

	setSize(WIDTH, HEIGHT);
}

void PluginBrowser::prepare(TrackId new_track, size_t new_position)
{
	track = new_track;
	position = new_position;
	search_field.setText({}, juce::dontSendNotification);
	search();
	search_field.grabKeyboardFocus();
}

void PluginBrowser::update()
{
	const auto progress = context.scanner.progress();

	if (progress.running == shown_scan_running && progress.done == shown_scan_done)
		return;

	shown_scan_running = progress.running;
	shown_scan_done = progress.done;
	search();
}

void PluginBrowser::search()
{
	const auto selected = list.getSelectedRow();
	const auto previous = selected >= 0 && selected < (int) results.size() ? results[(size_t) selected].identifier : juce::String();
	results = context.library.search(search_field.getText().trim());
	list.updateContent();

	auto row = 0;

	for (size_t index = 0; index < results.size(); ++index)
		if (results[index].identifier == previous)
			row = (int) index;

	list.selectRow(row);
	status.setText(describe_status(), juce::dontSendNotification);
	list.repaint();
}

juce::String PluginBrowser::describe_status() const
{
	const auto progress = context.scanner.progress();
	const auto count = juce::String((int) results.size()) + (results.size() == 1 ? " effect" : " effects");

	if (!progress.running)
		return count;

	return count + ", looking for new plugins (" + juce::String(progress.done) + "/" + juce::String(progress.total) + ")";
}

void PluginBrowser::choose(int row)
{
	if (row < 0 || row >= (int) results.size())
		return;

	const auto id = context.tracks.insert_plugin(track, results[(size_t) row], position);

	if (id.is_valid())
		context.selection.select_only(id);

	on_finished();
}

int PluginBrowser::getNumRows()
{
	return (int) results.size();
}

void PluginBrowser::paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool selected)
{
	if (row < 0 || row >= (int) results.size())
		return;

	const auto& plugin = results[(size_t) row];
	auto area = juce::Rectangle<int>(width, height).reduced(TEXT_MARGIN, 0);

	if (selected)
		graphics.fillAll(theme::ACCENT);

	graphics.setColour(selected ? theme::ACCENT_TEXT : theme::TEXT);
	graphics.setFont(juce::FontOptions(NAME_FONT_SIZE, juce::Font::bold));
	graphics.drawText(plugin.name, area.removeFromLeft((int) ((float) area.getWidth() * NAME_SHARE)), juce::Justification::centredLeft, true);

	graphics.setColour(selected ? theme::ACCENT_TEXT : theme::DIM_TEXT);
	graphics.setFont(juce::FontOptions(DETAIL_FONT_SIZE));
	graphics.drawText(plugin.manufacturer + "  " + plugin.format, area, juce::Justification::centredRight, true);
}

void PluginBrowser::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
	choose(row);
}

void PluginBrowser::returnKeyPressed(int row)
{
	choose(row);
}

bool PluginBrowser::keyPressed(const juce::KeyPress& key, juce::Component*)
{
	const auto step = key == juce::KeyPress::downKey ? 1 : (key == juce::KeyPress::upKey ? -1 : 0);

	if (step == 0 || results.empty())
		return false;

	list.selectRow(std::clamp(list.getSelectedRow() + step, 0, (int) results.size() - 1));
	return true;
}

void PluginBrowser::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds().reduced(PAGE_PADDING);

	search_field.setBounds(area.removeFromTop(ROW_HEIGHT));
	area.removeFromTop(ROW_GAP);

	auto status_row = area.removeFromBottom(ROW_HEIGHT);

	status_row.removeFromRight(BUTTON_GAP);
	cancel_button.setBounds(status_row.removeFromRight(BUTTON_WIDTH));

	status.setBounds(status_row);
	area.removeFromTop(ROW_GAP);

	list.setBounds(area);
}
