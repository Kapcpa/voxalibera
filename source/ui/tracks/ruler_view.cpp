#include "ui/tracks/ruler_view.h"

#include "ui/theme.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr int MIN_LABEL_SPACING = 48;
	constexpr int LABEL_PADDING = 4;
	constexpr int BAR_TICK_HEIGHT = 10;
	constexpr int STEP_TICK_HEIGHT = 5;
	constexpr float LABEL_FONT_SIZE = 12.0f;
	constexpr int GRID_NAME_WIDTH = 56;
	constexpr float EDGE_GRAB = 6.0f;
}

RulerView::RulerView(AppContext& context, const TimelineViewport& viewport) : context(context), viewport(viewport), layout{context, viewport}
{
}

void RulerView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::PANEL_BACKGROUND);
	paint_loop_range(graphics);

	for (const auto& line : layout.visible_grid_lines(getWidth())) {
		const auto tick_height = line.is_bar ? BAR_TICK_HEIGHT : STEP_TICK_HEIGHT;
		graphics.setColour(line.is_bar ? theme::DIM_TEXT : theme::GRID_BAR);
		graphics.drawVerticalLine((int) std::round(viewport.beat_to_x(line.beat)), (float) (getHeight() - tick_height), (float) getHeight());
	}

	paint_bar_labels(graphics);
	paint_grid_name(graphics);

	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(getHeight() - 1, 0.0f, (float) getWidth());
}

void RulerView::paint_loop_range(juce::Graphics& graphics)
{
	const auto& loop = context.project.loop;

	if (loop.end <= loop.start)
		return;

	const auto start_x = layout.tick_to_x(loop.start);
	const auto end_x = layout.tick_to_x(loop.end);

	graphics.setColour(loop.enabled ? theme::LOOP_RANGE : theme::TIME_SELECTION);
	graphics.fillRect(juce::Rectangle<float>(start_x, 0.0f, end_x - start_x, (float) getHeight()));
	graphics.setColour(loop.enabled ? theme::ACCENT : theme::DIM_TEXT);
	graphics.drawVerticalLine((int) std::round(start_x), 0.0f, (float) getHeight());
	graphics.drawVerticalLine((int) std::round(end_x), 0.0f, (float) getHeight());
}

void RulerView::paint_bar_labels(juce::Graphics& graphics)
{
	const auto beats_per_bar = context.project.tempo.signature.beats_per_bar;
	const auto bar_width = viewport.pixels_per_beat * beats_per_bar;
	const auto label_every = (int) juce::nextPowerOfTwo((int) std::ceil(MIN_LABEL_SPACING / bar_width));
	const auto first_bar = (int) std::floor(viewport.first_beat / beats_per_bar) / label_every * label_every;
	const auto last_bar = (int) std::ceil(viewport.x_to_beat(getWidth()) / beats_per_bar);

	graphics.setFont(juce::FontOptions(LABEL_FONT_SIZE));
	graphics.setColour(theme::TEXT);

	for (auto bar = first_bar; bar <= last_bar; bar += label_every) {
		const auto x = (float) viewport.beat_to_x(bar * beats_per_bar);
		graphics.drawText(juce::String(bar + 1), juce::Rectangle<float>(x + LABEL_PADDING, 0.0f, (float) MIN_LABEL_SPACING, (float) getHeight() - BAR_TICK_HEIGHT), juce::Justification::centredLeft);
	}
}

void RulerView::paint_grid_name(juce::Graphics& graphics)
{
	const auto area = getLocalBounds().removeFromRight(GRID_NAME_WIDTH).withTrimmedBottom(BAR_TICK_HEIGHT);
	const auto& grid = context.project.grid;

	graphics.setColour(theme::PANEL_BACKGROUND);
	graphics.fillRect(area);
	graphics.setFont(juce::FontOptions(LABEL_FONT_SIZE));
	graphics.setColour(grid.snap ? theme::TEXT : theme::DIM_TEXT);
	graphics.drawText(describe_grid_step(layout.grid_step(), context.project.tempo.signature), area.withTrimmedRight(LABEL_PADDING), juce::Justification::centredRight);
}

void RulerView::mouseDown(const juce::MouseEvent& event)
{
	const auto zone = zone_at(event.x);
	gesture = RulerGesture{zone, context.project.loop, layout.tick_at(event.x, event.mods), false};

	if (zone == RulerZone::select)
		context.locate_to_tick(gesture->grab_tick);
}

void RulerView::mouseDrag(const juce::MouseEvent& event)
{
	if (!gesture || !event.mouseWasDraggedSinceMouseDown())
		return;

	if (gesture->zone == RulerZone::select)
		drag_select(event);
	else
		drag_edge(event);
}

void RulerView::mouseUp(const juce::MouseEvent&)
{
	if (gesture && gesture->zone == RulerZone::select) {
		if (gesture->selected)
			context.locate_to_tick(context.project.loop.start);
		else
			context.clear_loop_range();
	}

	gesture.reset();
}

void RulerView::mouseMove(const juce::MouseEvent& event)
{
	const auto on_edge = zone_at(event.x) != RulerZone::select;
	setMouseCursor(on_edge ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::NormalCursor);
}

RulerView::RulerZone RulerView::zone_at(int x) const
{
	const auto& loop = context.project.loop;

	if (loop.end <= loop.start)
		return RulerZone::select;

	if (std::abs((float) x - layout.tick_to_x(loop.start)) <= EDGE_GRAB)
		return RulerZone::range_start;

	if (std::abs((float) x - layout.tick_to_x(loop.end)) <= EDGE_GRAB)
		return RulerZone::range_end;

	return RulerZone::select;
}

void RulerView::drag_select(const juce::MouseEvent& event)
{
	const auto tick = layout.tick_at(event.x, event.mods);

	if (std::abs(tick - gesture->grab_tick) < layout.grid_step() && !gesture->selected)
		return;

	gesture->selected = true;
	context.select_loop_range(gesture->grab_tick, tick);
}

void RulerView::drag_edge(const juce::MouseEvent& event)
{
	const auto tick = layout.tick_at(event.x, event.mods);
	const auto& original = gesture->original;
	const auto fixed_edge = gesture->zone == RulerZone::range_start ? original.end : original.start;

	if (tick != fixed_edge)
		context.select_loop_range(fixed_edge, tick);
}
