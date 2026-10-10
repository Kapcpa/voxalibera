#include "core/app_context.h"

AppContext::AppContext() : settings(Settings::load()), devices(engine, settings)
{
	engine.passthrough.monitoring = settings.monitoring;
	engine.passthrough.routing = settings.input_routing;
	devices.open();
}

void AppContext::update()
{
	const auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
	devices.update(now);
	xrun_watch.update(engine.device_manager.getCurrentAudioDevice(), now);
	publisher.update();
}

void AppContext::toggle_playback()
{
	if (engine.transport.is_playing())
		engine.transport.stop();
	else
		engine.transport.play();
}

void AppContext::return_to_start()
{
	engine.transport.locate(0);
}

void AppContext::toggle_metronome()
{
	project.set_metronome(!project.metronome);
}
