#pragma once

#include "core/device_session.h"
#include "core/settings.h"
#include "engine/audio_engine.h"

struct AppContext
{
	AppContext();

	void update();

	Settings settings;
	AudioEngine engine;
	DeviceSession devices;
};
