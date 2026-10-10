#pragma once

#include <juce_core/juce_core.h>

#include <vector>

constexpr int NO_CARD = -1;
constexpr int NO_FORCED_RATE = 0;

struct AlsaCard
{
	int index = NO_CARD;
	juce::String name;
};

std::vector<AlsaCard> list_alsa_cards();
juce::String find_card_name(const std::vector<AlsaCard>& cards, int card_index);
int find_card_for_alsa_device(const std::vector<AlsaCard>& cards, const juce::String& device_name);
int find_card_for_jack_device(const std::vector<AlsaCard>& cards, const juce::String& device_name);
int sample_rate_to_force(int card_index);
bool card_supports_rate(int card_index, int rate);
bool usb_stream_lists_rate(const juce::String& stream_description, int rate);
