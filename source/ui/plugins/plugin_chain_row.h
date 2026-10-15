#pragma once

#include "core/app_context.h"
#include "ui/plugins/plugin_slot.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class PluginChainRow : public juce::Component
{
public:
	explicit PluginChainRow(AppContext& context);

	void show(const Track& track);
	int preferred_width() const;

	void paint(juce::Graphics& graphics) override;
	void resized() override;
	void mouseMove(const juce::MouseEvent& event) override;
	void mouseExit(const juce::MouseEvent& event) override;
	void mouseDown(const juce::MouseEvent& event) override;

	std::function<void(size_t)> on_browse;
	std::function<void(PluginId)> on_open;

private:
	size_t gap_count() const { return slots.size() + 1; }
	juce::Rectangle<int> node_band() const;
	juce::Rectangle<int> input_area() const;
	juce::Rectangle<int> gap_area(size_t gap) const;
	juce::Rectangle<int> slot_area(size_t index) const;
	juce::Rectangle<int> output_area() const;
	juce::Rectangle<float> plus_area(size_t gap) const;
	std::optional<size_t> gap_at(juce::Point<int> position) const;
	std::optional<size_t> index_of(PluginId id) const;

	void rebuild(const Track& track);
	void click_slot(PluginId id);
	void drag_slot(const juce::MouseEvent& event);
	void drop_slot(PluginId id, const juce::MouseEvent& event);
	void set_hovered(std::optional<size_t> gap);

	void paint_end_node(juce::Graphics& graphics, juce::Rectangle<int> area, const juce::String& text, juce::Colour stripe) const;
	void paint_gap(juce::Graphics& graphics, size_t gap) const;

	AppContext& context;
	TrackId track_id;
	juce::String track_name;
	juce::Colour track_colour;
	std::vector<std::unique_ptr<PluginSlot>> slots;
	std::optional<size_t> hovered_gap;
	std::optional<size_t> drop_gap;
};
