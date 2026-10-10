#pragma once

#include "core/app_context.h"
#include "ui/panel_view.h"
#include "ui/transport_bar.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MainView : public juce::Component
{
public:
	explicit MainView(AppContext& context);

	void update();

	bool is_mixer_visible() const { return mixer.isVisible(); }
	void set_mixer_visible(bool visible);

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	TransportBar transport_bar;
	PanelView track_area{"Tracks"};
	PanelView mixer{"Mixer"};
};
