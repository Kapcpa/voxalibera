#pragma once

#include "core/device_session.h"
#include "core/engine_publisher.h"
#include "core/project.h"
#include "core/settings.h"
#include "core/xrun_watch.h"
#include "engine/audio_engine.h"

struct AppContext
{
	AppContext();

	void update();
	void toggle_playback();
	void return_to_start();
	void toggle_metronome();

	Settings settings;
	AudioEngine engine;
	DeviceSession devices;
	XrunWatch xrun_watch;
	Project project;
	EnginePublisher publisher{engine, project};
};
