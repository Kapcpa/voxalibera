#pragma once

#include "core/project/track.h"
#include "engine/musical_time.h"

#include <vector>

struct Project
{
	static constexpr double MIN_TEMPO = 20.0;
	static constexpr double MAX_TEMPO = 300.0;
	static constexpr int NOT_FOUND = -1;

	Track* find_track(TrackId id);
	const Track* find_track(TrackId id) const;
	int index_of(TrackId id) const;
	Clip* find_clip(ClipId id);
	const Clip* find_clip(ClipId id) const;
	Track* find_track_of(ClipId id);
	const Track* find_track_of(ClipId id) const;
	TrackId allocate_track_id();
	ClipId allocate_clip_id();

	juce::File folder;
	Tempo tempo;
	bool metronome = true;
	float master_volume_db = 0.0f;
	std::vector<Track> tracks;
	int next_id = 1;
	int revision = 0;
};
