#pragma once

#include "platform/alsa_cards.h"

#include <juce_core/juce_core.h>

#include <string>

struct sd_bus;
struct sd_bus_slot;

class DeviceReservation
{
public:
	DeviceReservation() = default;
	~DeviceReservation();

	juce::Result acquire(int card_index, const juce::String& application);
	void release();
	void process_requests();

	int card = NO_CARD;
	std::string application_name;
	std::string device_name;

private:
	juce::Result connect();
	juce::Result claim_name();
	juce::Result ask_owner_to_release();
	juce::Result take_over_name();

	sd_bus* bus = nullptr;
	sd_bus_slot* slot = nullptr;
	std::string bus_name;
	std::string object_path;

	JUCE_DECLARE_NON_COPYABLE(DeviceReservation)
};
