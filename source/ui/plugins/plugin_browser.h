#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

class PluginBrowser : public juce::Component, private juce::ListBoxModel, private juce::KeyListener
{
public:
	explicit PluginBrowser(AppContext& context);

	void prepare(TrackId track, size_t position);
	void update();
	void resized() override;

	std::function<void()> on_finished;

private:
	using juce::Component::keyPressed;

	int getNumRows() override;
	void paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool selected) override;
	void listBoxItemDoubleClicked(int row, const juce::MouseEvent& event) override;
	void returnKeyPressed(int row) override;
	bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

	void search();
	void choose(int row);
	juce::String describe_status() const;

	AppContext& context;
	TrackId track;
	size_t position = 0;
	std::vector<Plugin> results;
	int shown_scan_done = -1;
	bool shown_scan_running = false;

	juce::TextEditor search_field;
	juce::ListBox list{"Effects", this};
	juce::Label status;

	juce::TextButton cancel_button{"Cancel"};
};
