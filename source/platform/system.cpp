#include "platform/system.h"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

namespace
{
	constexpr auto CONFIG_HOME_VARIABLE = "XDG_CONFIG_HOME";
	constexpr auto DEFAULT_CONFIG_HOME = ".config";
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

	constexpr int AUDIO_THREAD_PRIORITY = 80;

	bool is_realtime_policy(int policy)
	{
		return policy == SCHED_FIFO || policy == SCHED_RR;
	}
}

juce::File app_config_directory()
{
	return xdg_directory(CONFIG_HOME_VARIABLE, DEFAULT_CONFIG_HOME).getChildFile(APPLICATION_NAME);
}

juce::File app_state_directory()
{
	return xdg_directory(STATE_HOME_VARIABLE, DEFAULT_STATE_HOME).getChildFile(APPLICATION_NAME);
}

juce::File app_data_directory()
{
	return xdg_directory(DATA_HOME_VARIABLE, DEFAULT_DATA_HOME).getChildFile(APPLICATION_NAME);
}

bool make_current_thread_realtime()
{
	sched_param parameters{};
	int policy = SCHED_OTHER;
	pthread_getschedparam(pthread_self(), &policy, &parameters);

	if (is_realtime_policy(policy))
		return true;

	parameters.sched_priority = AUDIO_THREAD_PRIORITY;
	return pthread_setschedparam(pthread_self(), SCHED_FIFO, &parameters) == 0;
}

int current_process_id()
{
	return (int) getpid();
}

bool is_process_running(int process_id, const juce::String& program_name)
{
	if (process_id <= 0)
		return false;

	const juce::File command("/proc/" + juce::String(process_id) + "/comm");
	return command.existsAsFile() && command.loadFileAsString().trim() == program_name;
}
