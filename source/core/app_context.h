#pragma once

#include "core/audio/device_session.h"
#include "core/audio/engine_publisher.h"
#include "core/audio/recorder.h"
#include "core/editors.h"
#include "core/log.h"
#include "core/media/peaks.h"
#include "core/media/playback_files.h"
#include "core/project/edit_history.h"
#include "core/project/model.h"
#include "core/project/project_session.h"
#include "core/selection.h"
#include "core/settings.h"
#include "engine/audio_engine.h"

struct AppContext
{
	AppContext();
	~AppContext();

	void update();
	void load_ahead_of(std::int64_t sample);
	void toggle_playback();
	void toggle_recording();
	void delete_selection();
	void return_to_start();
	void locate(std::int64_t sample);
	void toggle_metronome();
	void toggle_count_in();
	void set_master_volume(float volume_db);
	void set_tempo(double quarter_notes_per_minute);
	void set_time_signature(TimeSignature signature);
	void undo();
	void redo();

	Settings settings;
	juce::TimeSliceThread disk_thread{"Disk"};
	juce::TimeSliceThread recording_thread{"Recording writer"};
	AudioEngine engine;
	DeviceSession devices;
	XrunWatch xrun_watch;
	Project project;
	EditHistory edits{project};
	Selection selection;
	TrackEditor tracks{project, edits, selection};
	ClipEditor clips{project, edits, selection};
	PeakStore peaks;
	PeakBuilder peak_builder;
	ProjectSession session{engine, devices, project, edits, selection, peaks, peak_builder, disk_thread};
	Recorder recorder{engine, project, edits, selection, peaks, recording_thread};
	AudioFilePool files;
	ReadAhead read_ahead{engine.transport};
	EnginePublisher publisher{engine, project, files, read_ahead};
};
