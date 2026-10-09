#include "platform/device_reservation.h"

#include <systemd/sd-bus.h>

#include <cerrno>
#include <climits>
#include <cstring>
#include <thread>

namespace
{
constexpr auto RESERVATION_INTERFACE = "org.freedesktop.ReserveDevice1";
constexpr auto RESERVATION_NAME_PREFIX = "org.freedesktop.ReserveDevice1.Audio";
constexpr auto RESERVATION_PATH_PREFIX = "/org/freedesktop/ReserveDevice1/Audio";
constexpr int32_t RESERVATION_PRIORITY = INT32_MAX;
constexpr int TAKE_OVER_ATTEMPTS = 20;
constexpr auto TAKE_OVER_RETRY_DELAY = std::chrono::milliseconds(25);

juce::String describe_error(int status)
{
	return std::strerror(-status);
}

int read_property(sd_bus*, const char*, const char*, const char* property, sd_bus_message* reply, void* user_data, sd_bus_error*)
{
	const auto* reservation = static_cast<const DeviceReservation*>(user_data);

	if (std::strcmp(property, "Priority") == 0)
		return sd_bus_message_append(reply, "i", RESERVATION_PRIORITY);

	if (std::strcmp(property, "ApplicationName") == 0)
		return sd_bus_message_append(reply, "s", reservation->application_name.c_str());

	return sd_bus_message_append(reply, "s", reservation->device_name.c_str());
}

int refuse_release(sd_bus_message* message, void*, sd_bus_error*)
{
	return sd_bus_reply_method_return(message, "b", 0);
}

const sd_bus_vtable RESERVATION_VTABLE[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("RequestRelease", "i", "b", refuse_release, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_PROPERTY("Priority", "i", read_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("ApplicationName", "s", read_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("ApplicationDeviceName", "s", read_property, 0, SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_VTABLE_END};
}

DeviceReservation::~DeviceReservation()
{
	release();
}

juce::Result DeviceReservation::acquire(int card_index, const juce::String& application)
{
	release();

	bus_name = RESERVATION_NAME_PREFIX + std::to_string(card_index);
	object_path = RESERVATION_PATH_PREFIX + std::to_string(card_index);
	application_name = application.toStdString();
	device_name = "hw:" + std::to_string(card_index);

	auto result = connect();

	if (result.wasOk())
		result = claim_name();

	if (result.failed())
	{
		release();
		return result;
	}

	card = card_index;
	return juce::Result::ok();
}

juce::Result DeviceReservation::connect()
{
	if (const auto status = sd_bus_open_user(&bus); status < 0)
		return juce::Result::fail("cannot connect to the session bus: " + describe_error(status));

	if (const auto status = sd_bus_add_object_vtable(bus, &slot, object_path.c_str(), RESERVATION_INTERFACE, RESERVATION_VTABLE, this); status < 0)
		return juce::Result::fail("cannot publish the reservation: " + describe_error(status));

	return juce::Result::ok();
}

juce::Result DeviceReservation::claim_name()
{
	const auto status = sd_bus_request_name(bus, bus_name.c_str(), 0);

	if (status >= 0)
		return juce::Result::ok();

	if (status != -EEXIST)
		return juce::Result::fail("cannot claim " + juce::String(bus_name) + ": " + describe_error(status));

	if (const auto released = ask_owner_to_release(); released.failed())
		return released;

	return take_over_name();
}

juce::Result DeviceReservation::ask_owner_to_release()
{
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message* reply = nullptr;
	int granted = 0;

	const auto status = sd_bus_call_method(bus, bus_name.c_str(), object_path.c_str(), RESERVATION_INTERFACE, "RequestRelease", &error, &reply, "i", RESERVATION_PRIORITY);

	if (status >= 0)
		sd_bus_message_read(reply, "b", &granted);

	const juce::String reason = error.message != nullptr ? error.message : "";
	sd_bus_error_free(&error);
	sd_bus_message_unref(reply);

	if (granted)
		return juce::Result::ok();

	return juce::Result::fail(("the current owner refused to release it " + reason).trim());
}

juce::Result DeviceReservation::take_over_name()
{
	auto status = -EEXIST;

	for (int attempt = 0; attempt < TAKE_OVER_ATTEMPTS && status == -EEXIST; ++attempt)
	{
		status = sd_bus_request_name(bus, bus_name.c_str(), SD_BUS_NAME_REPLACE_EXISTING);

		if (status == -EEXIST)
			std::this_thread::sleep_for(TAKE_OVER_RETRY_DELAY);
	}

	if (status < 0)
		return juce::Result::fail("cannot claim " + juce::String(bus_name) + ": " + describe_error(status));

	return juce::Result::ok();
}

void DeviceReservation::release()
{
	if (bus != nullptr)
	{
		if (card != NO_CARD)
			sd_bus_release_name(bus, bus_name.c_str());

		slot = sd_bus_slot_unref(slot);
		bus = sd_bus_flush_close_unref(bus);
	}

	card = NO_CARD;
}

void DeviceReservation::process_requests()
{
	if (bus == nullptr)
		return;

	while (sd_bus_process(bus, nullptr) > 0)
	{
	}
}
