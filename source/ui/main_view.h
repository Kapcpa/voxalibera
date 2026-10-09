#pragma once

#include "ui/panel_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MainView : public juce::Component
{
public:
	MainView();

	bool is_mixer_visible() const { return mixer.isVisible(); }
	void set_mixer_visible(bool visible);

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	PanelView transport_bar{"Transport"};
	PanelView track_area{"Tracks"};
	PanelView mixer{"Mixer"};
};
