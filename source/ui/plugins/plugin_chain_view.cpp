#include "ui/plugins/plugin_chain_view.h"

#include "ui/theme.h"

namespace
{
	constexpr auto NO_TRACK_TEXT = "Select a track to see its effects";
	constexpr float HINT_FONT_SIZE = 13.0f;
	constexpr int MENU_WIDTH = 96;
	constexpr int MENU_PADDING = 10;
	constexpr int BUTTON_HEIGHT = 26;
	constexpr int BUTTON_GAP = 6;
}

PluginChainView::PluginChainView(AppContext& context) : context(context)
{
	row.on_browse = [this](size_t position) { browse(position); };
	row.on_open = [this](PluginId id) { open(id); };
	viewport.setViewedComponent(&row, false);
	viewport.setScrollBarsShown(false, true);
	addAndMakeVisible(viewport);

	add_button.onClick = [this] {
		if (const auto* track = shown_track())
			browse(insert_position(*track));
	};
	open_button.onClick = [this] { open(this->context.selection.plugin); };
	remove_button.onClick = [this] { this->context.remove_selected_plugin(); };
	bypass_button.onClick = [this] {
		if (const auto* track = shown_track())
			this->context.tracks.toggle(track->id, &Track::plugins_bypassed, "Bypass Effects");
	};
	bypass_button.setColour(juce::TextButton::buttonOnColourId, theme::BYPASS);
	bypass_button.setColour(juce::TextButton::textColourOnId, theme::WINDOW_BACKGROUND);

	for (auto* button : {&add_button, &open_button, &remove_button, &bypass_button})
		addAndMakeVisible(button);
}

const Track* PluginChainView::shown_track() const
{
	return context.selection.tracks.empty() ? nullptr : context.project.find_track(context.selection.tracks.front());
}

size_t PluginChainView::insert_position(const Track& track) const
{
	if (context.selection.gap)
		return *context.selection.gap;

	for (size_t index = 0; index < track.plugins.size(); ++index)
		if (track.plugins[index].id == context.selection.plugin)
			return index + 1;

	return track.plugins.size();
}

void PluginChainView::update()
{
	windows.update();

	if (!isVisible()) {
		shown_project_revision = -1;
		return;
	}

	const auto changed = context.project.revision != shown_project_revision || context.selection.revision != shown_selection_revision || context.plugins.revision() != shown_plugin_revision;

	if (!changed)
		return;

	shown_project_revision = context.project.revision;
	shown_selection_revision = context.selection.revision;
	shown_plugin_revision = context.plugins.revision();
	refresh();
}

void PluginChainView::refresh()
{
	const auto* track = shown_track();
	const auto plugin_selected = context.selection.plugin.is_valid();

	viewport.setVisible(track != nullptr);
	add_button.setEnabled(track != nullptr);
	open_button.setEnabled(plugin_selected && context.plugins.find_instance(context.selection.plugin) != nullptr);
	remove_button.setEnabled(plugin_selected);
	bypass_button.setEnabled(track != nullptr);
	bypass_button.setToggleState(track != nullptr && track->plugins_bypassed, juce::dontSendNotification);

	if (track != nullptr) {
		row.show(*track);
		resized();
	}

	repaint();
}

bool PluginChainView::activate_selection()
{
	if (!isVisible() || shown_track() == nullptr)
		return false;

	if (context.selection.plugin.is_valid()) {
		open(context.selection.plugin);
		return true;
	}

	if (context.selection.gap) {
		browse(*context.selection.gap);
		return true;
	}

	return false;
}

void PluginChainView::browse(size_t position)
{
	if (const auto* track = shown_track(); track != nullptr && on_browse != nullptr)
		on_browse(track->id, position);
}

void PluginChainView::open(PluginId id)
{
	windows.open(id);
}

void PluginChainView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(0, 0.0f, (float) getWidth());
	graphics.drawVerticalLine((float) getWidth() - MENU_WIDTH - MENU_PADDING, 0.0f, (float) getHeight());

	if (shown_track() != nullptr)
		return;

	graphics.setColour(theme::DIM_TEXT);
	graphics.setFont(juce::FontOptions(HINT_FONT_SIZE, juce::Font::bold));
	graphics.drawText(NO_TRACK_TEXT, getLocalBounds().withTrimmedRight(MENU_WIDTH), juce::Justification::centred, false);
}

void PluginChainView::resized()
{
	auto area = getLocalBounds();
	auto menu = area.removeFromRight(MENU_WIDTH).reduced(MENU_PADDING);

	for (auto* button : {&add_button, &open_button, &remove_button, &bypass_button}) {
		button->setBounds(menu.removeFromTop(BUTTON_HEIGHT));
		menu.removeFromTop(BUTTON_GAP);
	}

	viewport.setBounds(area);
	row.setSize(std::max(row.preferred_width(), viewport.getMaximumVisibleWidth()), viewport.getMaximumVisibleHeight());
}
