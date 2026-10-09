#include "core/device_session.h"

#include "core/hardware_restore.h"
#include "core/log.h"
#include "platform/pipewire_properties.h"

#include <set>

namespace
{
	constexpr auto APPLICATION_NAME = "custom-daw";
	constexpr int INPUT_CHANNELS = 2;
	constexpr int OUTPUT_CHANNELS = 2;
	constexpr double RESCAN_DELAY_SECONDS = 1.5;
}

DeviceSession::DeviceSession(AudioEngine& engine, Settings& settings) : engine(engine), settings(settings)
{
}

DeviceSession::~DeviceSession()
{
	engine.device_manager.closeAudioDevice();
	reservations.clear();
}

void DeviceSession::open()
{
	if (settings.device_type == ALSA)
		reserve_cards_for({settings.output_device, settings.input_device});

	if (settings.device_type == JACK)
		use_jack_properties_for(settings.output_device);

	engine.device_manager.initialise(INPUT_CHANNELS, OUTPUT_CHANNELS, nullptr, true);
	open_saved_device();
}

void DeviceSession::open_saved_device()
{
	const auto has_saved_device = settings.device_type.isNotEmpty();
	engine.device_manager.setCurrentAudioDeviceType(has_saved_device ? settings.device_type : juce::String(JACK), true);

	if (!has_saved_device)
		return;

	auto setup = engine.device_manager.getAudioDeviceSetup();
	setup.outputDeviceName = settings.output_device;
	setup.inputDeviceName = settings.input_device;
	setup.useDefaultInputChannels = true;
	setup.useDefaultOutputChannels = true;

	if (settings.sample_rate > 0.0)
		setup.sampleRate = settings.sample_rate;

	if (settings.buffer_size > 0)
		setup.bufferSize = settings.buffer_size;

	engine.device_manager.setAudioDeviceSetup(setup, true);
}

void DeviceSession::update(double now)
{
	for (auto& [card, reservation] : reservations)
		reservation->process_requests();

	if (const auto choice = current_choice(); choice != applied_choice)
		apply_choice(choice, now);

	restore_hardware_after_device_start();

	if (rescan_at && now >= *rescan_at) {
		rescan_at.reset();
		rescan_devices();
	}
}

void DeviceSession::rescan_devices()
{
	for (auto* type : engine.device_manager.getAvailableDeviceTypes())
		type->scanForDevices();

	engine.device_manager.sendChangeMessage();
}

DeviceSession::DeviceChoice DeviceSession::current_choice() const
{
	const auto setup = engine.device_manager.getAudioDeviceSetup();
	return {engine.device_manager.getCurrentAudioDeviceType(), setup.outputDeviceName, setup.inputDeviceName, setup.sampleRate, setup.bufferSize, settings.jack_buffer_size};
}

void DeviceSession::apply_choice(const DeviceChoice& choice, double now)
{
	const auto was_exclusive = is_exclusive();
	const auto is_alsa = choice.type == ALSA;
	const auto has_device = engine.device_manager.getCurrentAudioDevice() != nullptr;

	if (reserve_cards_for(is_alsa ? juce::StringArray{choice.output, choice.input} : juce::StringArray{}) && !has_device)
		reopen_device();

	if (!is_alsa && use_jack_properties_for(choice.output) && has_device)
		restart_device();

	if (was_exclusive && !is_exclusive())
		rescan_at = now + RESCAN_DELAY_SECONDS;

	const auto cards = list_alsa_cards();
	const auto& device_name = choice.output.isNotEmpty() ? choice.output : choice.input;
	active_card = is_alsa ? find_card_for_alsa_device(cards, device_name) : find_card_for_jack_device(cards, device_name);

	applied_choice = current_choice();
	log_device(applied_choice);

	if (engine.device_manager.getCurrentAudioDevice() != nullptr)
		remember_choice(applied_choice);
}

void DeviceSession::remember_choice(const DeviceChoice& choice)
{
	settings.device_type = choice.type;
	settings.output_device = choice.output;
	settings.input_device = choice.input;
	settings.sample_rate = choice.sample_rate;
	settings.buffer_size = choice.buffer_size;
	settings.save();
}

bool DeviceSession::reserve_cards_for(const juce::StringArray& alsa_device_names)
{
	const auto cards = list_alsa_cards();
	std::set<int> wanted_cards;

	for (const auto& name : alsa_device_names)
		if (const auto card = find_card_for_alsa_device(cards, name); card != NO_CARD)
			wanted_cards.insert(card);

	std::erase_if(reservations, [&](const auto& entry) {
		if (wanted_cards.contains(entry.first))
			return false;

		log_event("Returned " + find_card_name(cards, entry.first) + " to PipeWire");
		return true;
	});
	reservation_problems.clear();

	bool reserved_any = false;

	for (const auto card : wanted_cards) {
		if (reservations.contains(card))
			continue;

		auto reservation = std::make_unique<DeviceReservation>();

		if (const auto result = reservation->acquire(card, APPLICATION_NAME); result.failed()) {
			reservation_problems << find_card_name(cards, card) << ": " << result.getErrorMessage() << "\n";
			log_event("Could not borrow " + find_card_name(cards, card) + " from PipeWire: " + result.getErrorMessage());
			continue;
		}

		reservations[card] = std::move(reservation);
		log_event("Borrowed " + find_card_name(cards, card) + " from PipeWire");
		reserved_any = true;
	}

	return reserved_any;
}

bool DeviceSession::use_jack_properties_for(const juce::String& jack_device_name)
{
	const auto card = find_card_for_jack_device(list_alsa_cards(), jack_device_name);
	const auto forced_rate = card == NO_CARD ? NO_FORCED_RATE : sample_rate_to_force(card);
	const auto properties = jack_client_properties(settings.jack_buffer_size, forced_rate);

	if (properties == jack_properties)
		return false;

	use_jack_client_properties(properties);
	log_event("PipeWire client properties: " + properties);
	jack_properties = properties;
	return true;
}

void DeviceSession::reopen_device()
{
	engine.device_manager.setAudioDeviceSetup(engine.device_manager.getAudioDeviceSetup(), true);
}

void DeviceSession::restart_device()
{
	engine.device_manager.closeAudioDevice();
	engine.device_manager.restartLastAudioDevice();
}

void DeviceSession::restore_hardware_after_device_start()
{
	const auto device_starts = engine.device_starts.load();

	if (active_card == restored_card && device_starts == restored_device_starts)
		return;

	restored_card = active_card;
	restored_device_starts = device_starts;

	if (active_card == NO_CARD)
		return;

	if (const auto restored = restore_saved_hardware_controls(settings, active_card); restored > 0)
		log_event("Restored " + juce::String(restored) + " saved hardware controls for " + find_card_name(list_alsa_cards(), active_card));
}

void DeviceSession::log_device(const DeviceChoice& choice) const
{
	if (auto* device = engine.device_manager.getCurrentAudioDevice())
		log_event("Device open: " + choice.type + " / " + device->getName() + ", " + juce::String(device->getCurrentSampleRate(), 0) + " Hz, " + juce::String(device->getCurrentBufferSizeSamples()) + " samples");
	else
		log_event("No device open (" + choice.type + (choice.output.isEmpty() ? juce::String() : ", " + choice.output) + ")");
}
