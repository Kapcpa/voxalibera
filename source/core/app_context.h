#pragma once

#include "core/clip_editor.h"
#include "core/device_session.h"
#include "core/engine_publisher.h"
#include "core/media/audio_file_pool.h"
#include "core/media/peak_store.h"
#include "core/media/read_ahead.h"
#include "core/project/edit_history.h"
#include "core/project/model.h"
#include "core/recorder.h"
#include "core/selection.h"
#include "core/settings.h"
#include "core/track_editor.h"
#include "core/xrun_watch.h"
#include "engine/audio_engine.h"

struct AppContext
{
	AppContext();
	~AppContext();

	void update();
	void toggle_playback();
	void toggle_recording();
	void delete_selection();
	void return_to_start();
	void toggle_metronome();
	void set_master_volume(float volume_db);
	void set_tempo(double quarter_notes_per_minute);
	void set_time_signature(TimeSignature signature);
	void undo();
	void redo();

	Settings settings;
	juce::TimeSliceThread disk_thread{"Disk writer"};
	AudioEngine engine;
	DeviceSession devices;
	XrunWatch xrun_watch;
	Project project;
	EditHistory edits{project};
	Selection selection;
	TrackEditor tracks{project, edits, selection};
	ClipEditor clips{project, edits, selection};
	PeakStore peaks;
	Recorder recorder{engine, project, edits, selection, peaks, disk_thread};
	AudioFilePool files;
	ReadAhead read_ahead{engine.transport};
	EnginePublisher publisher{engine, project, files, read_ahead};
};
