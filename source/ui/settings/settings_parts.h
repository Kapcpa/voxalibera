#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace settings_layout
{
	constexpr int PAGE_PADDING = 24;
	constexpr int SECTION_LABEL_HEIGHT = 20;
	constexpr int SECTION_GAP = 24;
	constexpr int ROW_HEIGHT = 28;
	constexpr int ROW_GAP = 8;
	constexpr int LABEL_WIDTH = 120;
	constexpr int FIELD_WIDTH = 340;
	constexpr int BUTTON_WIDTH = 110;
}

class SettingsPage : public juce::Component
{
public:
	virtual void update() {}
};

class SectionLabel : public juce::Label
{
public:
	explicit SectionLabel(const juce::String& title);
};

class SidebarButton : public juce::Button
{
public:
	explicit SidebarButton(const juce::String& name);

	void paintButton(juce::Graphics& graphics, bool highlighted, bool pressed) override;
};
