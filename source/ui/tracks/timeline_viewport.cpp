#include "ui/tracks/timeline_viewport.h"

#include <algorithm>

void TimelineViewport::zoom_around(double x, double factor)
{
	const auto anchor_beat = x_to_beat(x);
	pixels_per_beat = std::clamp(pixels_per_beat * factor, MIN_PIXELS_PER_BEAT, MAX_PIXELS_PER_BEAT);
	first_beat = std::max(0.0, anchor_beat - x / pixels_per_beat);
	++revision;
}

void TimelineViewport::scroll_beats(double beats)
{
	first_beat = std::max(0.0, first_beat + beats);
	++revision;
}
