#pragma once

#include "core/project/model.h"

#include <juce_gui_basics/juce_gui_basics.h>

class PluginsButton : public juce::TextButton
{
public:
	PluginsButton();

	void show(const Track& track);
};
