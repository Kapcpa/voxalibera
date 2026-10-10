#include "core/engine_publisher.h"

#include "core/log.h"

#include <cmath>

namespace
{
	constexpr double MISSING_FILE_RETRY_SECONDS = 0.25;

	bool same_rate(double first, double second)
	{
		return std::lround(first) == std::lround(second);
	}
}

EnginePublisher::EnginePublisher(AudioEngine& engine, const Project& project, AudioFilePool& files, ReadAhead& read_ahead) : engine(engine), project(project), files(files), read_ahead(read_ahead)
{
}

void EnginePublisher::update(double now)
{
	const auto rate = engine.sample_rate.load();
	const auto retry_due = files_missing && now >= next_retry;

	if (project.revision != published_revision || !same_rate(rate, published_rate) || retry_due)
		publish(now);

	engine.snapshots.release_retired(engine.running.load());
}

int EnginePublisher::meter_of(TrackId id) const
{
	const auto found = meters.find(id);
	return found == meters.end() ? NO_METER : found->second;
}

void EnginePublisher::publish(double now)
{
	const auto rate = engine.sample_rate.load();
	const auto solo_active = any_solo(project.tracks);
	std::vector<ReadAheadRegion> regions;

	forget_removed_tracks();
	files_missing = false;

	auto snapshot = std::make_unique<EngineSnapshot>();
	snapshot->tempo = project.tempo;
	snapshot->metronome = project.metronome;
	snapshot->master_gain = fader_gain(project.master_volume_db);

	for (const auto& track : project.tracks)
		snapshot->tracks.push_back(build_track(track, solo_active, rate, regions));

	engine.snapshots.publish(std::move(snapshot));
	read_ahead.set_regions(std::move(regions), rate);

	published_revision = project.revision;
	published_rate = rate;
	next_retry = now + MISSING_FILE_RETRY_SECONDS;
}

TrackPlayback EnginePublisher::build_track(const Track& track, bool solo_active, double sample_rate, std::vector<ReadAheadRegion>& regions)
{
	TrackPlayback playback;
	playback.meter = assign_meter(track.id);
	playback.channels = channel_count(track.channels);
	playback.first_input = track.input.first_channel;
	playback.monitors_input = track.armed && track.input.is_connected();
	playback.audible = is_audible(track, solo_active);

	const auto gain = fader_gain(track.volume_db);
	const auto pan = playback.channels == 1 ? mono_pan_gains(track.pan) : stereo_balance_gains(track.pan);
	playback.gains = {gain * pan.left, gain * pan.right};

	for (const auto& clip : track.clips) {
		auto file = open_clip_file(clip, sample_rate);

		if (file == nullptr)
			continue;

		playback.clips.push_back({file, clip.start, clip.source_offset, clip.length, fader_gain(clip.gain_db)});
		regions.push_back({file, clip.start, clip.source_offset, clip.length});
	}

	return playback;
}

std::shared_ptr<const AudioFile> EnginePublisher::open_clip_file(const Clip& clip, double sample_rate)
{
	auto file = files.get(project.folder.getChildFile(clip.file), clip.source_offset + clip.length);

	if (file == nullptr) {
		files_missing = true;
		return nullptr;
	}

	if (same_rate(file->sample_rate(), sample_rate))
		return file;

	if (reported_rate_mismatches.insert(clip.file).second)
		log_event(clip.file + " is at " + juce::String(file->sample_rate(), 0) + " Hz but the device runs at " + juce::String(sample_rate, 0) + " Hz; it stays silent until they match");

	return nullptr;
}

int EnginePublisher::assign_meter(TrackId id)
{
	if (const auto found = meters.find(id); found != meters.end())
		return found->second;

	std::set<int> taken;

	for (const auto& [track, meter] : meters)
		taken.insert(meter);

	for (int meter = 0; meter < (int) MixRenderer::MAX_TRACK_METERS; ++meter)
		if (!taken.contains(meter))
			return meters[id] = meter;

	return NO_METER;
}

void EnginePublisher::forget_removed_tracks()
{
	std::erase_if(meters, [&](const auto& entry) { return project.find_track(entry.first) == nullptr; });
}
