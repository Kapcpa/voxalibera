#pragma once

#include "app/main_menu.h"
#include "app/settings_window.h"
#include "core/app_context.h"
#include "ui/main_view.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <map>
#include <memory>

class MainWindow : public juce::DocumentWindow, public juce::ApplicationCommandTarget, private juce::Timer
{
public:
	MainWindow(const juce::String& name, AppContext& context);
	~MainWindow() override;

	void closeButtonPressed() override;

	juce::ApplicationCommandTarget* getNextCommandTarget() override;
	void getAllCommands(juce::Array<juce::CommandID>& commands) override;
	void getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& info) override;
	bool perform(const InvocationInfo& info) override;

private:
	void timerCallback() override;
	void toggle_mixer();
	void show_audio_settings();
	void show_about();

	AppContext& context;
	juce::ApplicationCommandManager command_manager;
	MainMenu main_menu{command_manager};
	MainView main_view;
	SettingsWindow settings_window;
	std::map<juce::CommandID, std::function<void()>> actions;
};
