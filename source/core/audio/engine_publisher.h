#pragma once

#include "core/media/playback_files.h"
#include "core/plugins/plugin_pool.h"
#include "core/project/model.h"
#include "engine/audio_engine.h"

#include <functional>
#include <map>
#include <optional>
#include <set>

using ChainForTrack = std::function<std::shared_ptr<PluginChain>(const Track&)>;

struct SnapshotProblems
{
	bool files_missing = false;
	double mismatched_rate = 0.0;
};

class EnginePublisher
{
public:
	EnginePublisher(AudioEngine& engine, const Project& project, AudioFilePool& files, PluginPool& plugins, ReadAhead& read_ahead);

	void update(double now);
	std::unique_ptr<EngineSnapshot> build_snapshot(double rate, const ChainForTrack& chain_for, int resampler_taps, SnapshotProblems& problems);
	int meter_of(TrackId id) const;
	double mismatched_file_rate() const { return mismatched_rate; }

private:
	void publish(double now);
	TrackPlayback build_track(const Track& track, bool solo_active, double rate, const ChainForTrack& chain_for, int resampler_taps, SnapshotProblems& problems);
	std::optional<ClipPlayback> build_clip(const Clip& clip, double rate, int resampler_taps, SnapshotProblems& problems);
	std::shared_ptr<const ResamplingKernel> resampler_for(double step, int taps);
	int assign_meter(TrackId id);
	void forget_removed_tracks();

	AudioEngine& engine;
	const Project& project;
	AudioFilePool& files;
	PluginPool& plugins;
	ReadAhead& read_ahead;
	int published_revision = -1;
	int published_plugin_revision = 0;
	double published_rate = 0.0;
	bool files_missing = false;
	double mismatched_rate = 0.0;
	double next_retry = 0.0;
	std::map<TrackId, int> meters;
	std::set<juce::String> reported_rate_mismatches;
	std::map<std::pair<std::int64_t, int>, std::shared_ptr<const ResamplingKernel>> resamplers;
};
