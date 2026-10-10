#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class MainMenu : public juce::MenuBarModel
{
public:
	explicit MainMenu(juce::ApplicationCommandManager& command_manager);

	juce::StringArray getMenuBarNames() override;
	juce::PopupMenu getMenuForIndex(int menu_index, const juce::String& menu_name) override;
	void menuItemSelected(int item_id, int menu_index) override;

private:
	juce::PopupMenu::Item command_item(juce::CommandID id);

	juce::ApplicationCommandManager& command_manager;
};
