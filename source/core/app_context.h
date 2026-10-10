#pragma once

#include "core/device_session.h"
#include "core/engine_publisher.h"
#include "core/project/edit_history.h"
#include "core/project/project.h"
#include "core/selection.h"
#include "core/settings.h"
#include "core/track_editor.h"
#include "core/xrun_watch.h"
#include "engine/audio_engine.h"

struct AppContext
{
	AppContext();

	void update();
	void toggle_playback();
	void return_to_start();
	void toggle_metronome();
	void set_tempo(double quarter_notes_per_minute);
	void set_time_signature(TimeSignature signature);
	void undo();
	void redo();

	Settings settings;
	AudioEngine engine;
	DeviceSession devices;
	XrunWatch xrun_watch;
	Project project;
	EditHistory edits{project};
	Selection selection;
	TrackEditor tracks{project, edits, selection};
	EnginePublisher publisher{engine, project};
};
