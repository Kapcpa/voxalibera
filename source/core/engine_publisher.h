#pragma once

#include "core/media/audio_file_pool.h"
#include "core/media/read_ahead.h"
#include "core/project/model.h"
#include "engine/audio_engine.h"

#include <map>
#include <set>

class EnginePublisher
{
public:
	EnginePublisher(AudioEngine& engine, const Project& project, AudioFilePool& files, ReadAhead& read_ahead);

	void update(double now);
	int meter_of(TrackId id) const;
	double mismatched_file_rate() const { return mismatched_rate; }

private:
	void publish(double now);
	TrackPlayback build_track(const Track& track, bool solo_active, double sample_rate, std::vector<ReadAheadRegion>& regions);
	std::shared_ptr<const AudioFile> open_clip_file(const Clip& clip, double sample_rate);
	int assign_meter(TrackId id);
	void forget_removed_tracks();

	AudioEngine& engine;
	const Project& project;
	AudioFilePool& files;
	ReadAhead& read_ahead;
	int published_revision = -1;
	double published_rate = 0.0;
	bool files_missing = false;
	double mismatched_rate = 0.0;
	double next_retry = 0.0;
	std::map<TrackId, int> meters;
	std::set<juce::String> reported_rate_mismatches;
};
