#include "app/application.h"

#include "core/log.h"

#include <iostream>

namespace
{
	constexpr auto LIST_DEVICES_FLAG = "--list-devices";

	void print_device_names(juce::AudioIODeviceType& type, bool inputs)
	{
		for (const auto& name : type.getDeviceNames(inputs))
			std::cout << "  " << (inputs ? "in:  " : "out: ") << name << "\n";
	}

	void print_audio_devices()
	{
		juce::AudioDeviceManager device_manager;

		for (auto* type : device_manager.getAvailableDeviceTypes()) {
			type->scanForDevices();
			std::cout << type->getTypeName() << "\n";
			print_device_names(*type, false);
			print_device_names(*type, true);
		}
	}
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

	logger = create_file_logger(getApplicationName() + " " + getApplicationVersion() + " started");
	juce::Logger::setCurrentLogger(logger.get());
	juce::LookAndFeel::setDefaultLookAndFeel(&look_and_feel);
	context = std::make_unique<AppContext>();
	window = std::make_unique<MainWindow>(getApplicationName(), *context);
}

void Application::shutdown()
{
	window.reset();
	context.reset();
	juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
	log_event("Shut down");
	juce::Logger::setCurrentLogger(nullptr);
}

void Application::systemRequestedQuit()
{
	if (window != nullptr)
		window->request_quit();
	else
		quit();
}
