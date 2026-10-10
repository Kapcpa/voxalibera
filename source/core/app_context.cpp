#include "core/app_context.h"

AppContext::AppContext() : settings(Settings::load()), devices(engine, settings)
{
	engine.passthrough.monitoring = settings.monitoring;
	engine.passthrough.routing = settings.input_routing;
	disk_thread.startThread();
	devices.open();
}

void AppContext::update()
{
	const auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
	devices.update(now);
	xrun_watch.update(engine.device_manager.getCurrentAudioDevice(), now);
	recorder.update();
	publisher.update();
}

void AppContext::toggle_playback()
{
	if (recorder.is_recording())
		recorder.stop();
	else if (engine.transport.is_playing())
		engine.transport.stop();
	else
		engine.transport.play();
}

void AppContext::toggle_recording()
{
	if (recorder.is_recording())
		recorder.stop();
	else
		recorder.start();
}

void AppContext::delete_selection()
{
	if (!selection.clips.empty())
		clips.remove_selected();
	else
		tracks.remove_selected();
}

void AppContext::return_to_start()
{
	if (!recorder.is_recording())
		engine.transport.locate(0);
}

void AppContext::toggle_metronome()
{
	edits.begin(project.metronome ? "Metronome Off" : "Metronome On").set(&Project::metronome, !project.metronome);
}

void AppContext::set_tempo(double quarter_notes_per_minute)
{
	auto tempo = project.tempo;
	tempo.quarter_notes_per_minute = std::clamp(quarter_notes_per_minute, Project::MIN_TEMPO, Project::MAX_TEMPO);
	edits.begin("Tempo", "tempo").set(&Project::tempo, tempo);
}

void AppContext::set_time_signature(TimeSignature signature)
{
	auto tempo = project.tempo;
	tempo.signature = signature;
	edits.begin("Time Signature").set(&Project::tempo, tempo);
}

void AppContext::undo()
{
	edits.undo();
	selection.forget_missing(project);
}

void AppContext::redo()
{
	edits.redo();
	selection.forget_missing(project);
}
