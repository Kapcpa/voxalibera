#include "core/engine_publisher.h"

EnginePublisher::EnginePublisher(AudioEngine& engine, const Project& project) : engine(engine), project(project)
{
}

void EnginePublisher::update()
{
	if (project.revision != published_revision)
		publish();

	engine.snapshots.release_retired(engine.running.load());
}

void EnginePublisher::publish()
{
	auto snapshot = std::make_unique<EngineSnapshot>();
	snapshot->tempo = project.tempo;
	snapshot->metronome = project.metronome;

	engine.snapshots.publish(std::move(snapshot));
	published_revision = project.revision;
}
