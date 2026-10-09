#pragma once

#include <juce_core/juce_core.h>

#include <memory>

std::unique_ptr<juce::FileLogger> create_file_logger(const juce::String& first_line);
juce::File log_file();
void log_event(const juce::String& message);
