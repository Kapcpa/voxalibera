#include "core/audio/engine_publisher.h"

#include "core/log.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr double MISSING_FILE_RETRY_SECONDS = 0.25;
	constexpr std::int64_t ROUNDING_SAMPLES = 2;
	constexpr double STEP_KEY_SCALE = 1.0e9;
	constexpr double MICRO_FADE_SECONDS = 0.003;

	bool same_rate(double first, double second)
	{
		return std::lround(first) == std::lround(second);
	}
}

EnginePublisher::EnginePublisher(AudioEngine& engine, const Project& project, AudioFilePool& files, PluginPool& plugins, ReadAhead& read_ahead) : engine(engine), project(project), files(files), plugins(plugins), read_ahead(read_ahead)
{
}

void EnginePublisher::update(double now)
{
	const auto rate = engine.sample_rate.load();
	const auto retry_due = files_missing && now >= next_retry;

	if (project.revision != published_revision || plugins.revision() != published_plugin_revision || !same_rate(rate, published_rate) || retry_due)
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
	SnapshotProblems problems;

	forget_removed_tracks();
	plugins.use_rate(rate);
	plugins.forget_removed(project);

	auto snapshot = build_snapshot(rate, [this](const Track& track) { return plugins.chain_for(track); }, ResamplingKernel::REALTIME_TAPS, problems);
	std::vector<ReadAheadRegion> regions;

	for (size_t index = 0; index < snapshot->tracks.size(); ++index) {
		auto& track = snapshot->tracks[index];
		track.meter = assign_meter(project.tracks[index].id);

		for (const auto& clip : track.clips)
			regions.push_back({clip.file, clip.start, clip.length, clip.file_start, clip.resampler != nullptr ? clip.resampler->step() : 1.0});
	}

	engine.snapshots.publish(std::move(snapshot));
	read_ahead.set_regions(std::move(regions), rate);

	files_missing = problems.files_missing;
	mismatched_rate = problems.mismatched_rate;
	published_revision = project.revision;
	published_plugin_revision = plugins.revision();
	published_rate = rate;
	next_retry = now + MISSING_FILE_RETRY_SECONDS;
}

std::unique_ptr<EngineSnapshot> EnginePublisher::build_snapshot(double rate, const ChainForTrack& chain_for, int resampler_taps, SnapshotProblems& problems)
{
	const auto solo_active = any_solo(project.tracks);

	auto snapshot = std::make_unique<EngineSnapshot>();
	snapshot->tempo = project.tempo;
	snapshot->metronome = project.metronome;
	snapshot->master_gain = fader_gain(project.master_volume_db);

	for (const auto& track : project.tracks)
		snapshot->tracks.push_back(build_track(track, solo_active, rate, chain_for, resampler_taps, problems));

	return snapshot;
}

TrackPlayback EnginePublisher::build_track(const Track& track, bool solo_active, double rate, const ChainForTrack& chain_for, int resampler_taps, SnapshotProblems& problems)
{
	TrackPlayback playback;
	playback.channels = channel_count(track.channels);
	playback.first_input = track.input.first_channel;
	playback.monitors_input = track.armed && track.input.is_connected();
	playback.audible = is_audible(track, solo_active);

	const auto gain = fader_gain(track.volume_db);
	const auto pan = playback.channels == 1 ? mono_pan_gains(track.pan) : stereo_balance_gains(track.pan);
	playback.gains = {gain * pan.left, gain * pan.right};

	playback.plugins = chain_for(track);

	for (const auto& clip : track.clips)
		if (const auto built = build_clip(clip, rate, resampler_taps, problems))
			playback.clips.push_back(*built);

	return playback;
}

std::optional<ClipPlayback> EnginePublisher::build_clip(const Clip& clip, double rate, int resampler_taps, SnapshotProblems& problems)
{
	if (rate <= 0.0)
		return std::nullopt;

	const auto path = project.folder.getChildFile(clip.file);
	const auto timeline = timeline_rate(project, rate);
	auto file = files.get(path, 0);

	if (file != nullptr)
		file = files.get(path, std::max<std::int64_t>(0, convert_samples(clip.source_offset + clip.length, timeline, file->sample_rate()) - ROUNDING_SAMPLES));

	if (file == nullptr) {
		problems.files_missing = true;
		return std::nullopt;
	}

	const auto file_rate = file->sample_rate();
	const auto step = file_rate / rate;

	if (step > ResamplingKernel::MAX_STEP) {
		problems.mismatched_rate = file_rate;

		if (reported_rate_mismatches.insert(clip.file).second)
			log_event(clip.file + " is at " + juce::String(file_rate, 0) + " Hz, too far from " + juce::String(rate, 0) + " Hz to play");

		return std::nullopt;
	}

	ClipPlayback playback;
	playback.file = file;
	playback.start = convert_samples(clip.start, timeline, rate);
	playback.length = convert_samples(clip.length, timeline, rate);
	playback.file_start = (double) clip.source_offset * file_rate / timeline;
	playback.gain = fader_gain(clip.gain_db);
	playback.resampler = same_rate(file_rate, rate) ? nullptr : resampler_for(step, resampler_taps);

	const auto micro_fade = std::min((std::int64_t) (rate * MICRO_FADE_SECONDS), playback.length / 2);
	playback.fade_in = std::clamp(convert_samples(clip.fade_in, timeline, rate), micro_fade, playback.length - micro_fade);
	playback.fade_out = std::clamp(convert_samples(clip.fade_out, timeline, rate), micro_fade, playback.length - playback.fade_in);
	return playback;
}

std::shared_ptr<const ResamplingKernel> EnginePublisher::resampler_for(double step, int taps)
{
	auto& kernel = resamplers[{std::llround(step * STEP_KEY_SCALE), taps}];

	if (kernel == nullptr)
		kernel = std::make_shared<const ResamplingKernel>(step, taps);

	return kernel;
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
