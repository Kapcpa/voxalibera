#pragma once

#include <juce_core/juce_core.h>

constexpr auto APPLICATION_NAME = "voxalibera";

juce::File app_config_directory();
juce::File app_state_directory();
juce::File app_data_directory();
juce::File app_cache_directory();

bool make_current_thread_realtime();

int current_process_id();
bool is_process_running(int process_id, const juce::String& program_name);
