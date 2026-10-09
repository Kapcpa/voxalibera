#include "core/hardware_restore.h"

#include "platform/alsa_cards.h"
#include "platform/alsa_controls.h"

int restore_saved_hardware_controls(const Settings& settings, int card_index)
{
	const auto saved_card = settings.hardware_controls.find(find_card_name(list_alsa_cards(), card_index));

	if (saved_card == settings.hardware_controls.end())
		return 0;

	AlsaControls controls(card_index);
	int restored = 0;

	for (const auto& control : controls.controls) {
		const auto saved = saved_card->second.find(control.key);

		if (saved == saved_card->second.end())
			continue;

		controls.write(control, saved->second);
		++restored;
	}

	return restored;
}
