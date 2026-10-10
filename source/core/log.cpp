#include "core/log.h"

#include "platform/system.h"

namespace
{
	constexpr auto LOG_FILE = "voxalibera.log";
	constexpr juce::int64 MAX_LOG_BYTES = 1024 * 1024;
	constexpr auto TIME_FORMAT = "%H:%M:%S ";

	constexpr double SECONDS_BETWEEN_LOGS = 2.0;
}

juce::File log_file()
{
	return app_state_directory().getChildFile(LOG_FILE);
}

std::unique_ptr<juce::FileLogger> create_file_logger(const juce::String& first_line)
{
	log_file().getParentDirectory().createDirectory();
	return std::make_unique<juce::FileLogger>(log_file(), first_line, MAX_LOG_BYTES);
}

void log_event(const juce::String& message)
{
	juce::Logger::writeToLog(juce::Time::getCurrentTime().formatted(TIME_FORMAT) + message);
}

void XrunWatch::update(juce::AudioIODevice* device, double now)
{
	if (device != watched_device) {
		watched_device = device;
		logged_count = device != nullptr ? std::max(0, device->getXRunCount()) : 0;
		return;
	}

	if (device == nullptr || now < next_log_time)
		return;

	const auto count = device->getXRunCount();

	if (count <= logged_count)
		return;

	log_event("Xruns: " + juce::String(count - logged_count) + " new, " + juce::String(count) + " total");
	logged_count = count;
	next_log_time = now + SECONDS_BETWEEN_LOGS;
}
