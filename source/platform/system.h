#pragma once

#include <juce_core/juce_core.h>

juce::File state_directory();
juce::File data_directory();

bool make_current_thread_realtime();
