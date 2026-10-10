#pragma once

#include "core/project/project.h"
#include "engine/audio_engine.h"

class EnginePublisher
{
public:
	EnginePublisher(AudioEngine& engine, const Project& project);

	void update();

private:
	void publish();

	AudioEngine& engine;
	const Project& project;
	int published_revision = -1;
};
