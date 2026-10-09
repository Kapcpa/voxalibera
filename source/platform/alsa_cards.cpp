#include "platform/alsa_cards.h"

#include <alsa/asoundlib.h>

#include <optional>

namespace
{
	constexpr unsigned int PIPEWIRE_DEFAULT_RATE = 48000;

	const juce::StringArray GENERIC_DEVICE_WORDS{"audio", "stereo", "analog", "digital", "hd", "usb", "generic", "output", "input", "device", "pro", "mono", "surround"};

	struct CardRates
	{
		unsigned int lowest = 0;
		bool supports_pipewire_default = false;
	};

	juce::StringArray distinctive_words(const juce::String& name)
	{
		juce::StringArray words;
		words.addTokens(name.toLowerCase().replaceCharacters("-_,;:()", "       "), " ", {});
		words.removeEmptyStrings();

		for (const auto& word : GENERIC_DEVICE_WORDS)
			words.removeString(word);

		return words;
	}

	int count_shared_words(const juce::StringArray& first, const juce::StringArray& second)
	{
		int shared = 0;

		for (const auto& word : first)
			if (second.contains(word))
				++shared;

		return shared;
	}

	std::optional<CardRates> probe_card_rates(int card_index)
	{
		snd_pcm_t* pcm = nullptr;
		const auto device = "hw:" + juce::String(card_index) + ",0";

		if (snd_pcm_open(&pcm, device.toRawUTF8(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) != 0)
			return std::nullopt;

		snd_pcm_hw_params_t* parameters = nullptr;
		snd_pcm_hw_params_malloc(&parameters);

		std::optional<CardRates> rates;

		if (snd_pcm_hw_params_any(pcm, parameters) >= 0) {
			rates = CardRates{};
			rates->supports_pipewire_default = snd_pcm_hw_params_test_rate(pcm, parameters, PIPEWIRE_DEFAULT_RATE, 0) == 0;
			snd_pcm_hw_params_get_rate_min(parameters, &rates->lowest, nullptr);
		}

		snd_pcm_hw_params_free(parameters);
		snd_pcm_close(pcm);
		return rates;
	}

	CardRates read_running_rate(int card_index)
	{
		const juce::File hardware_parameters("/proc/asound/card" + juce::String(card_index) + "/pcm0p/sub0/hw_params");
		CardRates rates;

		for (const auto& line : juce::StringArray::fromLines(hardware_parameters.loadFileAsString()))
			if (line.startsWith("rate:"))
				rates.lowest = (unsigned int) line.fromFirstOccurrenceOf(":", false, false).trim().getIntValue();

		rates.supports_pipewire_default = rates.lowest == PIPEWIRE_DEFAULT_RATE;
		return rates;
	}
}

std::vector<AlsaCard> list_alsa_cards()
{
	std::vector<AlsaCard> cards;
	int index = NO_CARD;

	while (snd_card_next(&index) == 0 && index != NO_CARD) {
		char* name = nullptr;

		if (snd_card_get_name(index, &name) != 0)
			continue;

		cards.push_back({index, juce::String(name)});
		free(name);
	}

	return cards;
}

juce::String find_card_name(const std::vector<AlsaCard>& cards, int card_index)
{
	for (const auto& card : cards)
		if (card.index == card_index)
			return card.name;

	return {};
}

int find_card_for_alsa_device(const std::vector<AlsaCard>& cards, const juce::String& device_name)
{
	int best_card = NO_CARD;
	int best_length = 0;

	for (const auto& card : cards) {
		if (!device_name.startsWith(card.name) || card.name.length() <= best_length)
			continue;

		best_card = card.index;
		best_length = card.name.length();
	}

	return best_card;
}

int find_card_for_jack_device(const std::vector<AlsaCard>& cards, const juce::String& device_name)
{
	const auto device_words = distinctive_words(device_name);
	int best_card = NO_CARD;
	int best_score = 0;

	for (const auto& card : cards) {
		const auto score = count_shared_words(distinctive_words(card.name), device_words);

		if (score <= best_score)
			continue;

		best_card = card.index;
		best_score = score;
	}

	return best_card;
}

int sample_rate_to_force(int card_index)
{
	const auto probed_rates = probe_card_rates(card_index);
	const auto rates = probed_rates ? *probed_rates : read_running_rate(card_index);

	if (rates.lowest == 0 || rates.supports_pipewire_default)
		return NO_FORCED_RATE;

	return (int) rates.lowest;
}
