#pragma once

#include <juce_core/juce_core.h>

juce::File state_directory();
juce::File data_directory();

bool make_current_thread_realtime();

int current_process_id();
bool is_process_running(int process_id, const juce::String& program_name);
