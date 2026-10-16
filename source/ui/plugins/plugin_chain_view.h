#pragma once

#include "core/app_context.h"
#include "ui/plugins/plugin_chain_row.h"
#include "ui/plugins/plugin_window.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class PluginChainView : public juce::Component
{
public:
	explicit PluginChainView(AppContext& context);

	void update();
	bool activate_selection();

	void paint(juce::Graphics& graphics) override;
	void resized() override;

	std::function<void(TrackId, size_t)> on_browse;

private:
	const Track* shown_track() const;
	size_t insert_position(const Track& track) const;
	void browse(size_t position);
	void open(PluginId id);
	void refresh();

	AppContext& context;
	PluginChainRow row{context};
	juce::Viewport viewport;
	juce::TextButton add_button{"Add"};
	juce::TextButton open_button{"Open"};
	juce::TextButton remove_button{"Remove"};
	juce::TextButton bypass_button{"Bypass"};
	PluginWindows windows{context};
	int shown_project_revision = -1;
	int shown_selection_revision = -1;
	int shown_plugin_revision = -1;
};
