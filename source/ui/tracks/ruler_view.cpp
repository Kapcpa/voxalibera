#include "ui/tracks/ruler_view.h"

#include "ui/theme.h"

#include <cmath>

namespace
{
	constexpr int MIN_LABEL_SPACING = 48;
	constexpr int LABEL_PADDING = 4;
	constexpr int BAR_TICK_HEIGHT = 10;
	constexpr int BEAT_TICK_HEIGHT = 5;
	constexpr double MIN_BEAT_TICK_SPACING = 6.0;
	constexpr float LABEL_FONT_SIZE = 12.0f;
}

RulerView::RulerView(AppContext& context, const TimelineViewport& viewport) : context(context), viewport(viewport)
{
}

void RulerView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::PANEL_BACKGROUND);

	const auto beats_per_bar = context.project.tempo.signature.beats_per_bar;
	const auto bar_width = viewport.pixels_per_beat * beats_per_bar;
	const auto label_every = std::max(1, (int) std::ceil(MIN_LABEL_SPACING / bar_width));
	const auto first_beat = (int) std::floor(viewport.first_beat);
	const auto last_beat = (int) std::ceil(viewport.x_to_beat(getWidth()));

	graphics.setFont(juce::FontOptions(LABEL_FONT_SIZE));

	for (auto beat = first_beat; beat <= last_beat; ++beat) {
		const auto x = (float) viewport.beat_to_x(beat);
		const auto is_bar = beat % beats_per_bar == 0;

		if (!is_bar && viewport.pixels_per_beat < MIN_BEAT_TICK_SPACING)
			continue;

		graphics.setColour(is_bar ? theme::DIM_TEXT : theme::GRID_BAR);
		graphics.drawVerticalLine((int) x, (float) (getHeight() - (is_bar ? BAR_TICK_HEIGHT : BEAT_TICK_HEIGHT)), (float) getHeight());

		const auto bar = beat / beats_per_bar;

		if (is_bar && bar % label_every == 0) {
			graphics.setColour(theme::TEXT);
			graphics.drawText(juce::String(bar + 1), juce::Rectangle<float>(x + LABEL_PADDING, 0.0f, (float) MIN_LABEL_SPACING, (float) getHeight() - BAR_TICK_HEIGHT), juce::Justification::centredLeft);
		}
	}

	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(getHeight() - 1, 0.0f, (float) getWidth());
}

void RulerView::mouseDown(const juce::MouseEvent& event)
{
	locate_to(event.x);
}

void RulerView::mouseDrag(const juce::MouseEvent& event)
{
	locate_to(event.x);
}

void RulerView::locate_to(int x)
{
	const auto rate = context.engine.sample_rate.load();

	if (rate <= 0.0)
		return;

	const auto beat = std::max(0.0, std::round(viewport.x_to_beat(x)));
	context.locate(beat_start_sample((std::int64_t) beat, context.project.tempo, rate));
}
