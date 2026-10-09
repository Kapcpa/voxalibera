#include "platform/paths.h"

namespace
{
	constexpr auto STATE_HOME_VARIABLE = "XDG_STATE_HOME";
	constexpr auto DEFAULT_STATE_HOME = ".local/state";
}

juce::File state_directory()
{
	const auto state_home = juce::SystemStats::getEnvironmentVariable(STATE_HOME_VARIABLE, {});

	if (state_home.isNotEmpty())
		return juce::File(state_home);

	return juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile(DEFAULT_STATE_HOME);
}
