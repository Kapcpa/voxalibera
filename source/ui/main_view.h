#pragma once

#include "core/app_context.h"
#include "ui/mixer/mixer_view.h"
#include "ui/plugins/plugin_chain_view.h"
#include "ui/tracks/track_area_view.h"
#include "ui/transport_bar.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MainView : public juce::Component
{
public:
	explicit MainView(AppContext& context);

	void update();

	bool is_mixer_visible() const { return mixer.isVisible(); }
	void set_mixer_visible(bool visible);

	bool is_plugin_chain_visible() const { return plugin_chain.isVisible(); }
	void set_plugin_chain_visible(bool visible);

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	TransportBar transport_bar;

public:
	TrackAreaView track_area;
	PluginChainView plugin_chain;

private:
	MixerView mixer;
};
