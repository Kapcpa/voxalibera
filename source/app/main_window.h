#pragma once

#include "app/main_menu.h"
#include "app/project_dialogs.h"
#include "core/app_context.h"
#include "ui/main_view.h"
#include "ui/parts/message_dialog.h"
#include "ui/parts/modal_overlay.h"
#include "ui/plugins/plugin_browser.h"
#include "ui/settings/settings_view.h"
#include "ui/tracks/new_track_dialog.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <map>
#include <memory>

class WindowContent : public juce::Component
{
public:
	WindowContent(juce::MenuBarModel& menu, MainView& main_view, ModalOverlay& overlay);

	void resized() override;

private:
	juce::MenuBarComponent menu_bar;
	MainView& main_view;
	ModalOverlay& overlay;
};

class MainWindow : public juce::DocumentWindow, public juce::ApplicationCommandTarget, private juce::Timer
{
public:
	MainWindow(const juce::String& name, AppContext& context);
	~MainWindow() override;

	void closeButtonPressed() override;
	void request_quit();

	juce::ApplicationCommandTarget* getNextCommandTarget() override;
	void getAllCommands(juce::Array<juce::CommandID>& commands) override;
	void getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& info) override;
	bool perform(const InvocationInfo& info) override;

private:
	void timerCallback() override;
	bool is_allowed_now(juce::CommandID id) const;
	bool is_available(juce::CommandID id) const;
	void describe_dynamic_command(juce::CommandID id, juce::ApplicationCommandInfo& info) const;
	void show_new_track_dialog();
	void show_plugin_browser(TrackId track, size_t position);
	void close_overlay();
	void release_focus_outside_text_fields();
	void toggle_mixer();
	void toggle_plugin_chain();
	void show_plugins_of(TrackId track);
	void show_settings(int page);
	void show_about();
	void show_message(const juce::String& title, const juce::String& message, std::vector<MessageChoice> choices);
	void show_title();
	void quit_after_recording();

	AppContext& context;
	juce::ApplicationCommandManager command_manager;
	MainMenu main_menu{command_manager};
	MainView main_view;
	SettingsView settings_view;
	NewTrackDialog new_track_dialog;
	PluginBrowser plugin_browser;
	MessageDialog message_dialog;
	ProjectDialogs project_dialogs{context, [this](const auto& title, const auto& message, auto choices) { show_message(title, message, std::move(choices)); }};
	ModalOverlay overlay;
	WindowContent content{main_menu, main_view, overlay};
	std::map<juce::CommandID, std::function<void()>> actions;
	bool quit_pending = false;
};
