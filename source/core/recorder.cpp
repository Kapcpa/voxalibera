#include "core/recorder.h"

#include "core/log.h"
#include "core/media/take_files.h"

namespace
{
	constexpr int BITS_PER_SAMPLE = 24;
	constexpr double WRITE_BUFFER_SECONDS = 4.0;
	constexpr double FLUSH_SECONDS = 2.0;
}

Recorder::Recorder(AudioEngine& engine, Project& project, EditHistory& edits, Selection& selection, PeakStore& peak_store, juce::TimeSliceThread& disk_thread) : engine(engine), project(project), edits(edits), selection(selection), peak_store(peak_store), disk_thread(disk_thread)
{
}

std::int64_t Recorder::start_sample() const
{
	return session != nullptr ? session->start_sample.load(std::memory_order_acquire) : RecordingSession::NOT_STARTED;
}

std::vector<TrackId> Recorder::tracks_to_record()
{
	const auto armed_with_input = [&] {
		std::vector<TrackId> ids;

		for (const auto& track : project.tracks)
			if (track.armed && track.input.is_connected())
				ids.push_back(track.id);

		return ids;
	};

	if (const auto armed = armed_with_input(); !armed.empty() || selection.tracks.empty())
		return armed;

	auto edit = edits.begin("Arm Track");

	for (const auto id : selection.tracks)
		edit.set(id, &Track::armed, true);

	edit.commit();
	return armed_with_input();
}

std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> Recorder::create_writer(const juce::File& file, int channels, double sample_rate)
{
	std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();

	if (stream == nullptr)
		return nullptr;

	const auto options = juce::AudioFormatWriterOptions().withSampleRate(sample_rate).withNumChannels(channels).withBitsPerSample(BITS_PER_SAMPLE);
	auto writer = juce::WavAudioFormat().createWriterFor(stream, options);

	if (writer == nullptr)
		return nullptr;

	auto threaded = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(), disk_thread, (int) (WRITE_BUFFER_SECONDS * sample_rate));
	threaded->setFlushInterval((int) (FLUSH_SECONDS * sample_rate));
	return threaded;
}

std::unique_ptr<RecordingSession> Recorder::create_session(const std::vector<TrackId>& tracks, double sample_rate)
{
	auto new_session = std::make_unique<RecordingSession>();

	for (const auto id : tracks) {
		const auto& track = *project.find_track(id);
		const auto file = next_take_file(project.folder, track.name);
		auto take = std::make_unique<RecordingTake>();
		take->first_input = track.input.first_channel;
		take->channels = channel_count(track.channels);
		take->writer = create_writer(file, take->channels, sample_rate);

		if (take->writer == nullptr) {
			log_event("Could not create " + file.getFullPathName());
			return nullptr;
		}

		live.push_back({id, file, {RecordingTake::SAMPLES_PER_PEAK, std::vector<std::vector<float>>((size_t) take->channels)}});
		new_session->takes.push_back(std::move(take));
	}

	return new_session;
}

void Recorder::start()
{
	const auto sample_rate = engine.sample_rate.load();

	if (state != State::idle || !engine.running.load() || sample_rate <= 0.0)
		return;

	const auto tracks = tracks_to_record();

	if (tracks.empty())
		return;

	if (project.folder == juce::File())
		project.folder = new_unsaved_project_folder();

	auto new_session = create_session(tracks, sample_rate);

	if (new_session == nullptr) {
		live.clear();
		return;
	}

	session = new_session.get();
	engine.recording.publish(std::move(new_session));
	engine.transport.record();
	state = State::recording;
	log_event("Recording " + juce::String((int) tracks.size()) + " track(s) into " + project.folder.getFullPathName());
}

void Recorder::stop()
{
	if (state != State::recording)
		return;

	engine.transport.stop();
	state = State::finishing;
}

void Recorder::update()
{
	collect_peaks();

	if (state == State::finishing && (!engine.transport.is_recording() || !engine.running.load()))
		finish();

	engine.recording.release_retired(engine.running.load());
}

void Recorder::collect_peaks()
{
	if (session == nullptr)
		return;

	for (size_t position = 0; position < live.size(); ++position) {
		auto& take = live[position];
		float level = 0.0f;

		while (session->takes[position]->peaks.pop(level)) {
			take.peaks.channels[take.next_channel].push_back(level);
			take.next_channel = (take.next_channel + 1) % take.peaks.channels.size();
		}
	}
}

std::int64_t Recorder::device_latency() const
{
	auto* device = engine.device_manager.getCurrentAudioDevice();
	return device != nullptr ? device->getInputLatencyInSamples() + device->getOutputLatencyInSamples() : 0;
}

void Recorder::finish()
{
	collect_peaks();

	if (const auto start = start_sample(); start != RecordingSession::NOT_STARTED)
		add_recorded_clips(start);

	engine.recording.publish(nullptr);
	session = nullptr;
	live.clear();
	state = State::idle;
}

void Recorder::add_recorded_clips(std::int64_t start)
{
	const auto latency = device_latency();
	auto edit = edits.begin("Record");

	for (size_t position = 0; position < live.size(); ++position) {
		const auto& take = *session->takes[position];
		const auto written = take.written.load(std::memory_order_acquire);

		if (take.dropped_blocks.load() > 0)
			log_event("Recording dropped " + juce::String(take.dropped_blocks.load()) + " blocks: the disk could not keep up");

		if (written <= latency)
			continue;

		Clip clip;
		clip.file = relative_media_path(project.folder, live[position].file);
		clip.start = start;
		clip.source_offset = latency;
		clip.length = written - latency;

		peak_store.store(clip.file, std::move(live[position].peaks));
		edit.add_clip(live[position].track, clip);
	}
}
