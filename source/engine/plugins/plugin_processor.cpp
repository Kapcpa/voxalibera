#include "engine/plugins/plugin_processor.h"

#include <juce_gui_basics/juce_gui_basics.h>

std::unique_ptr<juce::Component> PluginProcessor::create_editor()
{
	return nullptr;
}
