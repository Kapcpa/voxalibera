#include "platform/pipewire_properties.h"

#include "platform/alsa_cards.h"

#include <cstdlib>

juce::String jack_client_properties(int quantum, int forced_sample_rate)
{
	auto properties = "{ node.force-quantum = " + juce::String(quantum);

	if (forced_sample_rate != NO_FORCED_RATE)
		properties << ", node.force-rate = " << forced_sample_rate;

	return properties + " }";
}

void use_jack_client_properties(const juce::String& properties)
{
	setenv("PIPEWIRE_PROPS", properties.toRawUTF8(), 1);
}
