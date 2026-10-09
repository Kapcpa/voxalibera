#include "core/log.h"

#include "platform/paths.h"

namespace
{
	constexpr auto APPLICATION_FOLDER = "voxalibera";
	constexpr auto LOG_FILE = "voxalibera.log";
	constexpr juce::int64 MAX_LOG_BYTES = 1024 * 1024;
	constexpr auto TIME_FORMAT = "%H:%M:%S ";
}

juce::File log_file()
{
	return state_directory().getChildFile(APPLICATION_FOLDER).getChildFile(LOG_FILE);
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
