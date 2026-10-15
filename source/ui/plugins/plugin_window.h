#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <map>
#include <memory>

class PluginWindow : public juce::DocumentWindow
{
public:
	PluginWindow(const juce::String& title, std::shared_ptr<PluginProcessor> processor, std::unique_ptr<juce::Component> editor);
	~PluginWindow() override;

	void closeButtonPressed() override;

	const std::shared_ptr<PluginProcessor> processor;
	bool closed = false;

private:
	std::unique_ptr<juce::Component> editor;
};

class PluginWindows
{
public:
	explicit PluginWindows(AppContext& context);

	void open(PluginId id);
	void update();

private:
	AppContext& context;
	std::map<PluginId, std::unique_ptr<PluginWindow>> windows;
};
