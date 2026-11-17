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
}

RulerView::RulerView(AppContext& context, const TimelineViewport& viewport) : context(context), viewport(viewport), layout{context, viewport}
{
}

void RulerView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::PANEL_BACKGROUND);

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
	locate_to(event);
}

void RulerView::mouseDrag(const juce::MouseEvent& event)
{
	locate_to(event);
}

void RulerView::locate_to(const juce::MouseEvent& event)
{
	const auto rate = context.engine.sample_rate.load();

	if (rate <= 0.0)
		return;

	const auto beat = std::max(0.0, layout.snap(viewport.x_to_beat(event.x), event.mods));
	context.locate(std::llround(beat * samples_per_beat(context.project.tempo, rate)));
}
