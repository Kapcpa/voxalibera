#include "platform/alsa_cards.h"
#include "platform/pipewire.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("jack client properties force the quantum")
{
	CHECK(jack_client_properties(256, NO_FORCED_RATE) == "{ node.force-quantum = 256 }");
}

TEST_CASE("jack client properties force the rate only when needed")
{
	CHECK(jack_client_properties(128, 44100) == "{ node.force-quantum = 128, node.force-rate = 44100 }");
}
