#include "app/main_menu.h"

#include "app/commands.h"
#include "app/shortcut_text.h"

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
		{"Transport", {command::PLAY_STOP, command::RETURN_TO_START, command::TOGGLE_METRONOME}},
		{"Track", {command::NEW_TRACK, command::DELETE_TRACKS, SEPARATOR, command::ARM_TRACKS, command::MUTE_TRACKS, command::SOLO_TRACKS}},
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
			popup.addItem(command_item(item));
	}

	return popup;
}

juce::PopupMenu::Item MainMenu::command_item(juce::CommandID id)
{
	juce::ApplicationCommandInfo info(id);
	const auto* target = command_manager.getTargetForCommand(id, info);
	const auto keys = command_manager.getKeyMappings()->getKeyPressesAssignedToCommand(id);

	juce::PopupMenu::Item item(info.shortName);
	item.itemID = id;
	item.commandManager = &command_manager;
	item.isEnabled = target != nullptr && (info.flags & juce::ApplicationCommandInfo::isDisabled) == 0;
	item.isTicked = (info.flags & juce::ApplicationCommandInfo::isTicked) != 0;
	item.shortcutKeyDescription = keys.isEmpty() ? juce::String() : describe_shortcut(keys.getFirst());
	return item;
}

void MainMenu::menuItemSelected(int, int)
{
}
