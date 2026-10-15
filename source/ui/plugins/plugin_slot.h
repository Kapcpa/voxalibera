#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class PluginSlot : public juce::Component
{
public:
	PluginSlot(AppContext& context, const Plugin& plugin);

	void show(const Plugin& plugin, bool selected, bool missing);
	PluginId id() const { return plugin_data.id; }

	void paint(juce::Graphics& graphics) override;
	void resized() override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;

	std::function<void()> on_click;
	std::function<void(const juce::MouseEvent&)> on_drag;
	std::function<void(const juce::MouseEvent&)> on_drop;

private:
	void show_bypass();

	AppContext& context;
	Plugin plugin_data;
	bool is_selected = false;
	bool is_missing = false;
	bool dragging = false;

	juce::TextButton bypass_button;
	juce::TextButton remove_button;
};
