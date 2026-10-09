#include "core/xrun_watch.h"

#include "core/log.h"

namespace
{
	constexpr double SECONDS_BETWEEN_LOGS = 2.0;
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
