#pragma once

#include "core/project/model.h"

#include <juce_core/juce_core.h>

juce::String impulse_wildcard();
juce::String import_impulse_file(Project& project, const juce::File& file);
