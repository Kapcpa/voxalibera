#include "ui/plugins/plugin_chain_row.h"

#include "ui/theme.h"

namespace
{
	constexpr auto OUTPUT_TEXT = "Output";
	constexpr int PADDING = 12;
	constexpr int MAX_NODE_HEIGHT = 96;
	constexpr int END_NODE_WIDTH = 110;
	constexpr int SLOT_WIDTH = 130;
	constexpr int GAP_WIDTH = 44;
	constexpr int STRIPE_WIDTH = 4;
	constexpr int END_TEXT_MARGIN = 10;
	constexpr int END_TEXT_LINES = 2;
	constexpr float END_FONT_SIZE = 13.0f;
	constexpr float CORNER_RADIUS = 4.0f;
	constexpr float BORDER_WIDTH = 1.0f;
	constexpr float WIRE_WIDTH = 2.0f;
	constexpr float ARROW_SIZE = 6.0f;
	constexpr float PLUS_RADIUS = 9.0f;
	constexpr float PLUS_ARM = 4.0f;
	constexpr float DROP_MARKER_WIDTH = 3.0f;
}

PluginChainRow::PluginChainRow(AppContext& context) : context(context)
{
}

void PluginChainRow::show(const Track& track)
{
	const auto same_plugins = track.id == track_id && track.plugins.size() == slots.size() && std::equal(track.plugins.begin(), track.plugins.end(), slots.begin(), [](const Plugin& plugin, const auto& slot) { return plugin.id == slot->id(); });

	if (!same_plugins) {
		rebuild(track);
		resized();
	}

	track_name = track.name;
	track_colour = juce::Colour(track.colour);

	for (size_t index = 0; index < slots.size(); ++index) {
		const auto& plugin = track.plugins[index];
		slots[index]->show(plugin, context.selection.plugin == plugin.id, context.plugins.is_missing(plugin.id));
	}

	repaint();
}

void PluginChainRow::rebuild(const Track& track)
{
	track_id = track.id;
	slots.clear();
	hovered_gap.reset();
	drop_gap.reset();

	for (const auto& plugin : track.plugins) {
		auto slot = std::make_unique<PluginSlot>(context, plugin);
		const auto id = plugin.id;
		slot->on_click = [this, id] { click_slot(id); };
		slot->on_drag = [this](const juce::MouseEvent& event) { drag_slot(event); };
		slot->on_drop = [this, id](const juce::MouseEvent& event) { drop_slot(id, event); };
		addAndMakeVisible(*slot);
		slots.push_back(std::move(slot));
	}
}

int PluginChainRow::preferred_width() const
{
	return output_area().getRight() + PADDING;
}

juce::Rectangle<int> PluginChainRow::node_band() const
{
	const auto height = std::min(getHeight() - 2 * PADDING, MAX_NODE_HEIGHT);
	return {0, (getHeight() - height) / 2, getWidth(), height};
}

juce::Rectangle<int> PluginChainRow::input_area() const
{
	return node_band().withX(PADDING).withWidth(END_NODE_WIDTH);
}

juce::Rectangle<int> PluginChainRow::gap_area(size_t gap) const
{
	return node_band().withX(input_area().getRight() + (int) gap * (GAP_WIDTH + SLOT_WIDTH)).withWidth(GAP_WIDTH);
}

juce::Rectangle<int> PluginChainRow::slot_area(size_t index) const
{
	return node_band().withX(gap_area(index).getRight()).withWidth(SLOT_WIDTH);
}

juce::Rectangle<int> PluginChainRow::output_area() const
{
	return node_band().withX(gap_area(slots.size()).getRight()).withWidth(END_NODE_WIDTH);
}

juce::Rectangle<float> PluginChainRow::plus_area(size_t gap) const
{
	return juce::Rectangle<float>(2 * PLUS_RADIUS, 2 * PLUS_RADIUS).withCentre(gap_area(gap).getCentre().toFloat());
}

std::optional<size_t> PluginChainRow::gap_at(juce::Point<int> position) const
{
	for (size_t gap = 0; gap < gap_count(); ++gap)
		if (gap_area(gap).withY(0).withHeight(getHeight()).contains(position))
			return gap;

	return std::nullopt;
}

std::optional<size_t> PluginChainRow::index_of(PluginId id) const
{
	for (size_t index = 0; index < slots.size(); ++index)
		if (slots[index]->id() == id)
			return index;

	return std::nullopt;
}

void PluginChainRow::resized()
{
	for (size_t index = 0; index < slots.size(); ++index)
		slots[index]->setBounds(slot_area(index));
}

void PluginChainRow::paint(juce::Graphics& graphics)
{
	paint_end_node(graphics, input_area(), track_name, track_colour);
	paint_end_node(graphics, output_area(), OUTPUT_TEXT, theme::DIM_TEXT);

	for (size_t gap = 0; gap < gap_count(); ++gap)
		paint_gap(graphics, gap);
}

void PluginChainRow::paint_end_node(juce::Graphics& graphics, juce::Rectangle<int> area, const juce::String& text, juce::Colour stripe) const
{
	const auto bounds = area.toFloat();
	graphics.setColour(theme::PANEL_BACKGROUND);
	graphics.fillRoundedRectangle(bounds, CORNER_RADIUS);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawRoundedRectangle(bounds, CORNER_RADIUS, BORDER_WIDTH);
	graphics.setColour(stripe);
	graphics.fillRect(area.removeFromLeft(STRIPE_WIDTH));
	graphics.setColour(theme::TEXT);
	graphics.setFont(juce::FontOptions(END_FONT_SIZE, juce::Font::bold));
	graphics.drawFittedText(text, area.reduced(END_TEXT_MARGIN), juce::Justification::centredLeft, END_TEXT_LINES);
}

void PluginChainRow::paint_gap(juce::Graphics& graphics, size_t gap) const
{
	const auto area = gap_area(gap).toFloat();
	const auto selected = context.selection.gap == gap;
	const auto hovered = hovered_gap == gap;
	const auto colour = selected ? theme::ACCENT : (hovered ? theme::TEXT : theme::DIM_TEXT);
	const auto y = area.getCentreY();

	graphics.setColour(colour);
	graphics.drawLine(area.getX(), y, area.getRight() - ARROW_SIZE, y, WIRE_WIDTH);

	juce::Path arrow;
	arrow.addTriangle(area.getRight() - ARROW_SIZE, y - ARROW_SIZE / 2, area.getRight() - ARROW_SIZE, y + ARROW_SIZE / 2, area.getRight(), y);
	graphics.fillPath(arrow);

	if (drop_gap == gap) {
		graphics.setColour(theme::ACCENT);
		graphics.fillRect(juce::Rectangle<float>(DROP_MARKER_WIDTH, area.getHeight()).withCentre(area.getCentre()));
		return;
	}

	if (!selected && !hovered)
		return;

	const auto plus = plus_area(gap);
	graphics.setColour(selected ? theme::ACCENT : theme::WIDGET_BACKGROUND);
	graphics.fillEllipse(plus);
	graphics.setColour(selected ? theme::ACCENT_TEXT : theme::TEXT);
	graphics.drawLine(plus.getCentreX() - PLUS_ARM, plus.getCentreY(), plus.getCentreX() + PLUS_ARM, plus.getCentreY(), WIRE_WIDTH);
	graphics.drawLine(plus.getCentreX(), plus.getCentreY() - PLUS_ARM, plus.getCentreX(), plus.getCentreY() + PLUS_ARM, WIRE_WIDTH);
}

void PluginChainRow::set_hovered(std::optional<size_t> gap)
{
	if (gap == hovered_gap)
		return;

	hovered_gap = gap;
	repaint();
}

void PluginChainRow::mouseMove(const juce::MouseEvent& event)
{
	set_hovered(gap_at(event.getPosition()));
}

void PluginChainRow::mouseExit(const juce::MouseEvent&)
{
	set_hovered(std::nullopt);
}

void PluginChainRow::mouseDown(const juce::MouseEvent& event)
{
	const auto gap = gap_at(event.getPosition());

	if (!gap) {
		context.selection.clear_chain();
		return;
	}

	const auto on_plus = plus_area(*gap).contains(event.position);
	const auto already_selected = context.selection.gap == gap;
	context.selection.select_gap(*gap);

	if (on_plus || already_selected)
		on_browse(*gap);
}

void PluginChainRow::click_slot(PluginId id)
{
	if (context.selection.plugin == id) {
		on_open(id);
		return;
	}

	context.selection.select_only(id);
}

void PluginChainRow::drag_slot(const juce::MouseEvent& event)
{
	const auto gap = gap_at(event.getEventRelativeTo(this).getPosition());

	if (gap == drop_gap)
		return;

	drop_gap = gap;
	repaint();
}

void PluginChainRow::drop_slot(PluginId id, const juce::MouseEvent& event)
{
	const auto gap = gap_at(event.getEventRelativeTo(this).getPosition());
	const auto index = index_of(id);
	drop_gap.reset();
	repaint();

	if (!gap || !index)
		return;

	context.tracks.move_plugin(id, track_id, *gap > *index ? *gap - 1 : *gap);
}
