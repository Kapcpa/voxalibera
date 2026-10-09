#include "platform/alsa_controls.h"

#include <alsa/asoundlib.h>

#include <optional>
#include <set>

namespace
{
const juce::StringArray TEST_SIGNAL_CONTROL_WORDS{"Impulse"};

bool is_test_signal_control(const juce::String& name)
{
	for (const auto& word : TEST_SIGNAL_CONTROL_WORDS)
		if (name.containsIgnoreCase(word))
			return true;

	return false;
}

bool is_user_adjustable(snd_ctl_elem_info_t* info)
{
	return snd_ctl_elem_info_get_interface(info) == SND_CTL_ELEM_IFACE_MIXER && snd_ctl_elem_info_is_writable(info) && !snd_ctl_elem_info_is_inactive(info);
}

std::optional<AlsaControlKind> kind_of(snd_ctl_elem_type_t type)
{
	if (type == SND_CTL_ELEM_TYPE_INTEGER)
		return AlsaControlKind::level;

	if (type == SND_CTL_ELEM_TYPE_ENUMERATED)
		return AlsaControlKind::choice;

	if (type == SND_CTL_ELEM_TYPE_BOOLEAN)
		return AlsaControlKind::toggle;

	return std::nullopt;
}

juce::StringArray read_choices(snd_ctl_t* handle, snd_ctl_elem_info_t* info)
{
	juce::StringArray choices;
	const auto count = snd_ctl_elem_info_get_items(info);

	for (unsigned int item = 0; item < count; ++item)
	{
		snd_ctl_elem_info_set_item(info, item);
		snd_ctl_elem_info(handle, info);
		choices.add(snd_ctl_elem_info_get_item_name(info));
	}

	return choices;
}
}

AlsaControls::AlsaControls(int card_index)
{
	const auto device = "hw:" + juce::String(card_index);

	if (card_index == NO_CARD || snd_ctl_open(&handle, device.toRawUTF8(), 0) != 0)
		return;

	card = card_index;
	load_controls();
}

AlsaControls::~AlsaControls()
{
	if (handle != nullptr)
		snd_ctl_close(handle);
}

void AlsaControls::load_controls()
{
	snd_ctl_elem_list_t* list = nullptr;
	snd_ctl_elem_info_t* info = nullptr;
	snd_ctl_elem_list_malloc(&list);
	snd_ctl_elem_info_malloc(&info);

	snd_ctl_elem_list(handle, list);
	snd_ctl_elem_list_alloc_space(list, snd_ctl_elem_list_get_count(list));
	snd_ctl_elem_list(handle, list);

	std::set<juce::String> seen_names;

	for (unsigned int position = 0; position < snd_ctl_elem_list_get_used(list); ++position)
	{
		snd_ctl_elem_info_set_numid(info, snd_ctl_elem_list_get_numid(list, position));

		if (snd_ctl_elem_info(handle, info) < 0 || !is_user_adjustable(info))
			continue;

		const juce::String name(snd_ctl_elem_info_get_name(info));
		const auto kind = kind_of(snd_ctl_elem_info_get_type(info));

		if (!kind || is_test_signal_control(name))
			continue;

		AlsaControl control;
		control.key = seen_names.insert(name).second ? name : name + "#" + juce::String(snd_ctl_elem_info_get_index(info));
		control.id = snd_ctl_elem_info_get_numid(info);
		control.channel_count = snd_ctl_elem_info_get_count(info);
		control.kind = *kind;

		if (control.kind == AlsaControlKind::level)
		{
			control.minimum = snd_ctl_elem_info_get_min(info);
			control.maximum = snd_ctl_elem_info_get_max(info);
		}

		if (control.kind == AlsaControlKind::choice)
			control.choices = read_choices(handle, info);

		if (control.kind == AlsaControlKind::level && control.maximum <= control.minimum)
			continue;

		controls.push_back(control);
	}

	snd_ctl_elem_info_free(info);
	snd_ctl_elem_list_free_space(list);
	snd_ctl_elem_list_free(list);
}

int AlsaControls::read(const AlsaControl& control) const
{
	snd_ctl_elem_value_t* value = nullptr;
	snd_ctl_elem_value_malloc(&value);
	snd_ctl_elem_value_set_numid(value, control.id);

	int result = 0;

	if (snd_ctl_elem_read(handle, value) == 0)
	{
		switch (control.kind)
		{
			case AlsaControlKind::level:
				result = (int) snd_ctl_elem_value_get_integer(value, 0);
				break;
			case AlsaControlKind::choice:
				result = (int) snd_ctl_elem_value_get_enumerated(value, 0);
				break;
			case AlsaControlKind::toggle:
				result = snd_ctl_elem_value_get_boolean(value, 0);
				break;
		}
	}

	snd_ctl_elem_value_free(value);
	return result;
}

void AlsaControls::write(const AlsaControl& control, int new_value)
{
	snd_ctl_elem_value_t* value = nullptr;
	snd_ctl_elem_value_malloc(&value);
	snd_ctl_elem_value_set_numid(value, control.id);

	for (unsigned int channel = 0; channel < control.channel_count; ++channel)
	{
		switch (control.kind)
		{
			case AlsaControlKind::level:
				snd_ctl_elem_value_set_integer(value, channel, new_value);
				break;
			case AlsaControlKind::choice:
				snd_ctl_elem_value_set_enumerated(value, channel, (unsigned int) new_value);
				break;
			case AlsaControlKind::toggle:
				snd_ctl_elem_value_set_boolean(value, channel, new_value);
				break;
		}
	}

	snd_ctl_elem_write(handle, value);
	snd_ctl_elem_value_free(value);
}
