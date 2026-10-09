#include "app/main_menu.h"

#include "app/commands.h"

#include <vector>

namespace
{
	constexpr juce::CommandID SEPARATOR = 0;

	struct Menu
	{
		juce::String name;
		std::vector<juce::CommandID> items;
	};

	const std::vector<Menu> MENUS{
		{"File", {command::NEW_PROJECT, command::OPEN_PROJECT, SEPARATOR, command::SAVE_PROJECT, command::SAVE_PROJECT_AS, SEPARATOR, command::QUIT}},
		{"Edit", {command::UNDO, command::REDO}},
		{"View", {command::TOGGLE_MIXER}},
		{"Settings", {command::AUDIO_SETTINGS, command::INTERFACE_SETTINGS}},
		{"Help", {command::ABOUT}},
	};
}

MainMenu::MainMenu(juce::ApplicationCommandManager& command_manager) : command_manager(command_manager)
{
	setApplicationCommandManagerToWatch(&command_manager);
}

juce::StringArray MainMenu::getMenuBarNames()
{
	juce::StringArray names;

	for (const auto& menu : MENUS)
		names.add(menu.name);

	return names;
}

juce::PopupMenu MainMenu::getMenuForIndex(int menu_index, const juce::String&)
{
	juce::PopupMenu popup;

	for (const auto item : MENUS[(size_t) menu_index].items) {
		if (item == SEPARATOR)
			popup.addSeparator();
		else
			popup.addCommandItem(&command_manager, item);
	}

	return popup;
}

void MainMenu::menuItemSelected(int, int)
{
}
