#pragma once

#include "core/device_session.h"
#include "core/settings.h"
#include "core/xrun_watch.h"
#include "engine/audio_engine.h"

struct AppContext
{
	AppContext();

	void update();

	Settings settings;
	AudioEngine engine;
	DeviceSession devices;
	XrunWatch xrun_watch;
};
