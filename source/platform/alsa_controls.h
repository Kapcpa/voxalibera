#pragma once

#include "platform/alsa_cards.h"

#include <juce_core/juce_core.h>

#include <vector>

struct _snd_ctl;

enum class AlsaControlKind
{
	level,
	choice,
	toggle
};

struct AlsaControl
{
	juce::String key;
	unsigned int id = 0;
	unsigned int channel_count = 1;
	AlsaControlKind kind = AlsaControlKind::level;
	long minimum = 0;
	long maximum = 0;
	juce::StringArray choices;
};

class AlsaControls
{
public:
	explicit AlsaControls(int card_index);
	~AlsaControls();

	bool is_open() const { return handle != nullptr; }
	int read(const AlsaControl& control) const;
	void write(const AlsaControl& control, int value);

	int card = NO_CARD;
	std::vector<AlsaControl> controls;

private:
	void load_controls();

	_snd_ctl* handle = nullptr;

	JUCE_DECLARE_NON_COPYABLE(AlsaControls)
};
