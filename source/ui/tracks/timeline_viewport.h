#pragma once

struct TimelineViewport
{
	static constexpr double MIN_PIXELS_PER_BEAT = 2.0;
	static constexpr double MAX_PIXELS_PER_BEAT = 400.0;
	static constexpr int TRACK_HEIGHT = 64;

	double beat_to_x(double beat) const { return (beat - first_beat) * pixels_per_beat; }
	double x_to_beat(double x) const { return first_beat + x / pixels_per_beat; }
	void zoom_around(double x, double factor);
	void scroll_beats(double beats);

	double pixels_per_beat = 24.0;
	double first_beat = 0.0;
	int scroll_y = 0;
	int revision = 0;
};
