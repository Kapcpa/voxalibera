#include "ui/tracks/timeline_layout.h"

#include <algorithm>

namespace
{
	constexpr float CLIP_MARGIN = 3.0f;
}

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

double TimelineLayout::sample_rate() const
{
	return timeline_rate(context.project, context.engine.sample_rate.load());
}

double TimelineLayout::sample_to_x(std::int64_t sample) const
{
	const auto rate = sample_rate();
	return rate > 0.0 ? viewport.beat_to_x((double) sample / samples_per_beat(context.project.tempo, rate)) : 0.0;
}

juce::Rectangle<float> TimelineLayout::clip_area(size_t track_index, std::int64_t start, std::int64_t length) const
{
	const auto top = (float) ((int) track_index * TimelineViewport::TRACK_HEIGHT - viewport.scroll_y) + CLIP_MARGIN;
	const auto left = (float) sample_to_x(start);
	return {left, top, (float) sample_to_x(start + length) - left, (float) TimelineViewport::TRACK_HEIGHT - 2.0f * CLIP_MARGIN};
}

std::optional<ClipUnder> TimelineLayout::clip_at(juce::Point<int> position) const
{
	const auto& tracks = context.project.tracks;

	for (size_t index = 0; index < tracks.size(); ++index)
		for (const auto& clip : tracks[index].clips)
			if (const auto area = clip_area(index, clip.start, clip.length); area.contains(position.toFloat()))
				return ClipUnder{clip.id, area};

	return std::nullopt;
}

int TimelineLayout::track_index_at(int y) const
{
	return (y + viewport.scroll_y) / TimelineViewport::TRACK_HEIGHT;
}
