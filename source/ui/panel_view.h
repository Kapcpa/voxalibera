#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class PanelView : public juce::Component
{
public:
	explicit PanelView(const juce::String& title);

	void paint(juce::Graphics& graphics) override;

private:
	juce::String title;
};
