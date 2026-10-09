#include "platform/alsa_cards.h"

#include <catch2/catch_test_macros.hpp>

namespace
{
	const std::vector<AlsaCard> CARDS{
		{0, "HDA NVidia"},
		{1, "HD-Audio Generic"},
		{3, "POD Studio UX1"},
	};
}

TEST_CASE("alsa device names map to the card they start with")
{
	CHECK(find_card_for_alsa_device(CARDS, "POD Studio UX1 (direct hardware)") == 3);
	CHECK(find_card_for_alsa_device(CARDS, "HD-Audio Generic, ALC256 Analog; Front output / input") == 1);
	CHECK(find_card_for_alsa_device(CARDS, "Default ALSA Output (currently PipeWire Media Server)") == NO_CARD);
}

TEST_CASE("jack device names map by distinctive words")
{
	CHECK(find_card_for_jack_device(CARDS, "UX1 Stereo") == 3);
	CHECK(find_card_for_jack_device(CARDS, "Ryzen HD Audio Controller Analog Stereo") == NO_CARD);
}
