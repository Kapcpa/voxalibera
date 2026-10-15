#pragma once

#include "core/app_context.h"
#include "ui/settings/audio_page.h"
#include "ui/settings/interface_page.h"
#include "ui/settings/plugin_page.h"
#include "ui/settings/settings_parts.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class SettingsView : public juce::Component
{
public:
	static constexpr int AUDIO_PAGE = 0;
	static constexpr int INTERFACE_PAGE = 1;
	static constexpr int PLUGIN_PAGE = 2;

	explicit SettingsView(AppContext& context);

	void show_page(int page);
	void update();

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	AudioPage audio_page;
	InterfacePage interface_page;
	PluginPage plugin_page;
	SidebarButton audio_button{"Audio"};
	SidebarButton interface_button{"Interface"};
	SidebarButton plugin_button{"Plugins"};
	std::array<SettingsPage*, 3> pages{&audio_page, &interface_page, &plugin_page};
	std::array<SidebarButton*, 3> buttons{&audio_button, &interface_button, &plugin_button};
	int current_page = AUDIO_PAGE;
};
