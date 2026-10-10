#pragma once

#include "core/media/peaks.h"
#include "core/project/edit_history.h"
#include "core/selection.h"
#include "engine/audio_engine.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <memory>
#include <vector>

struct LiveTake
{
	TrackId track;
	juce::File file;
	Peaks peaks;
	size_t next_channel = 0;
};

class Recorder
{
public:
	Recorder(AudioEngine& engine, Project& project, EditHistory& edits, Selection& selection, PeakStore& peak_store, juce::TimeSliceThread& disk_thread);

	void start();
	void stop();
	void update();

	bool is_recording() const { return state != State::idle; }
	std::int64_t start_sample() const;
	const std::vector<LiveTake>& live_takes() const { return live; }

private:
	enum class State
	{
		idle,
		recording,
		finishing,
	};

	std::vector<TrackId> tracks_to_record();
	std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> create_writer(const juce::File& file, int channels, double sample_rate);
	std::unique_ptr<RecordingSession> create_session(const std::vector<TrackId>& tracks, double sample_rate);
	void collect_peaks();
	void finish();
	void add_recorded_clips(std::int64_t start);
	std::int64_t device_latency() const;

	AudioEngine& engine;
	Project& project;
	EditHistory& edits;
	Selection& selection;
	PeakStore& peak_store;
	juce::TimeSliceThread& disk_thread;
	State state = State::idle;
	RecordingSession* session = nullptr;
	std::vector<LiveTake> live;
};
