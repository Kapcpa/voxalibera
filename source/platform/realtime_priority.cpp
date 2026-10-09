#include "platform/realtime_priority.h"

#include <pthread.h>
#include <sched.h>

namespace
{
	constexpr int AUDIO_THREAD_PRIORITY = 80;

	bool is_realtime_policy(int policy)
	{
		return policy == SCHED_FIFO || policy == SCHED_RR;
	}
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
