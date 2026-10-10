#include "platform/paths.h"

namespace
{
	constexpr auto STATE_HOME_VARIABLE = "XDG_STATE_HOME";
	constexpr auto DEFAULT_STATE_HOME = ".local/state";
	constexpr auto DATA_HOME_VARIABLE = "XDG_DATA_HOME";
	constexpr auto DEFAULT_DATA_HOME = ".local/share";

	juce::File xdg_directory(const char* variable, const char* default_relative_to_home)
	{
		const auto configured = juce::SystemStats::getEnvironmentVariable(variable, {});

		if (configured.isNotEmpty())
			return juce::File(configured);

		return juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(default_relative_to_home);
	}
}

juce::File state_directory()
{
	return xdg_directory(STATE_HOME_VARIABLE, DEFAULT_STATE_HOME);
}

juce::File data_directory()
{
	return xdg_directory(DATA_HOME_VARIABLE, DEFAULT_DATA_HOME);
}
