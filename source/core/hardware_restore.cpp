#include "core/hardware_restore.h"

#include "platform/alsa_cards.h"
#include "platform/alsa_controls.h"

void restore_saved_hardware_controls(const Settings& settings, int card_index)
{
	const auto saved_card = settings.hardware_controls.find(find_card_name(list_alsa_cards(), card_index));

	if (saved_card == settings.hardware_controls.end())
		return;

	AlsaControls controls(card_index);

	for (const auto& control : controls.controls)
		if (const auto saved = saved_card->second.find(control.key); saved != saved_card->second.end())
			controls.write(control, saved->second);
}
