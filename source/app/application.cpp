#include "app/application.h"

#include "app/device_listing.h"

namespace
{
	constexpr auto LIST_DEVICES_FLAG = "--list-devices";
}

const juce::String Application::getApplicationName()
{
	return JUCE_APPLICATION_NAME_STRING;
}

const juce::String Application::getApplicationVersion()
{
	return JUCE_APPLICATION_VERSION_STRING;
}

bool Application::moreThanOneInstanceAllowed()
{
	return false;
}

void Application::initialise(const juce::String& command_line)
{
	if (command_line.contains(LIST_DEVICES_FLAG)) {
		print_audio_devices();
		quit();
		return;
	}

	context = std::make_unique<AppContext>();
	window = std::make_unique<MainWindow>(getApplicationName(), *context);
}

void Application::shutdown()
{
	window.reset();
	context.reset();
}

void Application::systemRequestedQuit()
{
	quit();
}
