#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SidebarButton : public juce::Button
{
public:
	explicit SidebarButton(const juce::String& name);

	void paintButton(juce::Graphics& graphics, bool highlighted, bool pressed) override;
};
