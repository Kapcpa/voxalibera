#pragma once

#include <juce_core/juce_core.h>

juce::String jack_client_properties(int quantum, int forced_sample_rate);
void use_jack_client_properties(const juce::String& properties);
