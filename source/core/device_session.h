#pragma once

#include "core/settings.h"
#include "engine/audio_engine.h"
#include "platform/alsa_cards.h"
#include "platform/device_reservation.h"

#include <map>
#include <memory>
#include <optional>

class DeviceSession
{
public:
	static constexpr auto ALSA = "ALSA";
	static constexpr auto JACK = "JACK";

	DeviceSession(AudioEngine& engine, Settings& settings);
	~DeviceSession();

	void open();
	void update(double now);
	void rescan_devices();

	bool is_exclusive() const { return !reservations.empty(); }

	int active_card = NO_CARD;
	juce::String jack_properties;
	juce::String reservation_problems;

private:
	struct DeviceChoice
	{
		juce::String type;
		juce::String output;
		juce::String input;
		double sample_rate = 0.0;
		int buffer_size = 0;
		int jack_buffer_size = 0;

		bool operator==(const DeviceChoice&) const = default;
	};

	DeviceChoice current_choice() const;
	void open_saved_device();
	void apply_choice(const DeviceChoice& choice, double now);
	void remember_choice(const DeviceChoice& choice);
	bool reserve_cards_for(const juce::StringArray& alsa_device_names);
	bool use_jack_properties_for(const juce::String& jack_device_name);
	void reopen_device();
	void restart_device();
	void restore_hardware_after_device_start();

	AudioEngine& engine;
	Settings& settings;
	std::map<int, std::unique_ptr<DeviceReservation>> reservations;
	DeviceChoice applied_choice;
	std::optional<double> rescan_at;
	int restored_card = NO_CARD;
	int restored_device_starts = 0;
};
