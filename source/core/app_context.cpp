#include "core/app_context.h"

AppContext::AppContext() : settings(Settings::load()), devices(engine, settings)
{
	engine.passthrough.monitoring = settings.monitoring;
	engine.passthrough.routing = settings.input_routing;
	devices.open();
}

void AppContext::update()
{
	devices.update(juce::Time::getMillisecondCounterHiRes() / 1000.0);
}
