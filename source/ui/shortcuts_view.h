#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

struct ShortcutGroup
{
	juce::String title;
	std::vector<std::pair<juce::String, juce::String>> rows;
};

class ShortcutsView : public juce::Component
{
public:
	ShortcutsView();

	void show(std::vector<ShortcutGroup> keyboard_groups);
	void resized() override;

private:
	class List : public juce::Component
	{
	public:
		void paint(juce::Graphics& graphics) override;
		int content_height() const;

		std::vector<ShortcutGroup> groups;
	};

	juce::Viewport viewport;
	List list;
};
