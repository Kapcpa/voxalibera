#include "core/app_context.h"

namespace
{
	constexpr double LOAD_SECONDS = 0.5;
}

AppContext::AppContext() : settings(Settings::load()), devices(engine, settings)
{
	engine.passthrough.monitoring = settings.test_monitor;
	engine.passthrough.routing = settings.input_routing;
	disk_thread.addTimeSliceClient(&read_ahead);
	disk_thread.startThread();
	recording_thread.startThread(juce::Thread::Priority::high);
	devices.open();
	scanner.start(false);
}

AppContext::~AppContext()
{
	disk_thread.removeTimeSliceClient(&read_ahead);
}

void AppContext::update()
{
	const auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
	devices.update(now);
	xrun_watch.update(engine.device_manager.getCurrentAudioDevice(), now);
	recorder.update();
	peak_builder.deliver_to(peaks);

	if (scanner.update())
		plugins.retry_missing();

	if (plugins.take_changes())
		edits.mark_changed_without_undo();

	session.update(now);
	publisher.update(now);
}

void AppContext::toggle_playback()
{
	if (recorder.is_recording()) {
		recorder.stop();
		return;
	}

	if (engine.transport.is_playing()) {
		engine.transport.stop();
		return;
	}

	load_ahead_of(engine.transport.position());
	engine.transport.play();
}

void AppContext::toggle_recording()
{
	if (recorder.is_recording()) {
		recorder.stop();
		return;
	}

	load_ahead_of(engine.transport.position() - recorder.count_in_samples(engine.sample_rate.load()));
	recorder.start();
}

void AppContext::delete_selection()
{
	if (selection.plugin.is_valid()) {
		remove_selected_plugin();
		return;
	}

	if (!selection.clips.empty())
		clips.remove_selected();
	else
		tracks.remove_selected();
}

void AppContext::remove_selected_plugin()
{
	const auto* track = project.find_track_of(selection.plugin);

	if (track == nullptr)
		return;

	const auto position = (size_t) (project.find_plugin(selection.plugin) - track->plugins.data());
	tracks.remove_plugin(selection.plugin);
	selection.select_gap(position);
}

void AppContext::return_to_start()
{
	if (!recorder.is_recording())
		locate(0);
}

void AppContext::locate(std::int64_t sample)
{
	load_ahead_of(sample);
	engine.transport.locate(sample);
}

void AppContext::load_ahead_of(std::int64_t sample)
{
	read_ahead.load_now(sample, (std::int64_t) (LOAD_SECONDS * engine.sample_rate.load()));
}

void AppContext::toggle_metronome()
{
	edits.set_without_undo(&Project::metronome, !project.metronome);
}

void AppContext::toggle_count_in()
{
	edits.set_without_undo(&Project::count_in, !project.count_in);
}

void AppContext::set_master_volume(float volume_db)
{
	tracks.set_master_volume(volume_db);
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
