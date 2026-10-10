#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class MainMenu : public juce::MenuBarModel
{
public:
	explicit MainMenu(juce::ApplicationCommandManager& command_manager);

	juce::StringArray getMenuBarNames() override;
	juce::PopupMenu getMenuForIndex(int menu_index, const juce::String& menu_name) override;
	void menuItemSelected(int item_id, int menu_index) override;

	std::function<juce::Array<juce::File>()> recent_files;
	std::function<bool()> can_open_recent;
	std::function<void(const juce::File&)> on_open_recent;

private:
	juce::PopupMenu::Item command_item(juce::CommandID id);
	juce::PopupMenu::Item recent_menu();

	juce::Array<juce::File> shown_recent_files;

	juce::ApplicationCommandManager& command_manager;
};
