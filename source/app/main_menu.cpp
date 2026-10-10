#include "app/main_menu.h"

#include "app/commands.h"

#include <utility>
#include <vector>

namespace
{
	const std::vector<std::pair<int, juce::String>> NAMED_KEYS{
		{juce::KeyPress::spaceKey, "Space"},
		{juce::KeyPress::returnKey, "Enter"},
		{juce::KeyPress::escapeKey, "Esc"},
		{juce::KeyPress::tabKey, "Tab"},
		{juce::KeyPress::backspaceKey, "Backspace"},
		{juce::KeyPress::deleteKey, "Delete"},
		{juce::KeyPress::insertKey, "Insert"},
		{juce::KeyPress::homeKey, "Home"},
		{juce::KeyPress::endKey, "End"},
		{juce::KeyPress::pageUpKey, "Page Up"},
		{juce::KeyPress::pageDownKey, "Page Down"},
		{juce::KeyPress::leftKey, "Left"},
		{juce::KeyPress::rightKey, "Right"},
		{juce::KeyPress::upKey, "Up"},
		{juce::KeyPress::downKey, "Down"},
	};

	constexpr int FUNCTION_KEY_COUNT = 12;

	juce::String describe_key(int key_code)
	{
		for (const auto& [code, name] : NAMED_KEYS)
			if (code == key_code)
				return name;

		for (int number = 1; number <= FUNCTION_KEY_COUNT; ++number)
			if (key_code == juce::KeyPress::F1Key + number - 1)
				return "F" + juce::String(number);

		return juce::String::charToString((juce::juce_wchar) key_code).toUpperCase();
	}

	constexpr juce::CommandID SEPARATOR = 0;
	constexpr juce::CommandID RECENT_MENU = -1;
	constexpr int FIRST_RECENT_ITEM = 10000;

	juce::String describe_recent(const juce::File& file)
	{
		const auto home = juce::File::getSpecialLocation(juce::File::userHomeDirectory);
		const auto folder = file.getParentDirectory();
		const auto location = folder.isAChildOf(home) ? "~/" + folder.getRelativePathFrom(home) : folder.getFullPathName();
		return file.getFileNameWithoutExtension() + "   " + location;
	}

	struct Menu
	{
		juce::String name;
		std::vector<juce::CommandID> items;
	};

	const std::vector<Menu> MENUS{
		{"File", {command::NEW_PROJECT, command::OPEN_PROJECT, RECENT_MENU, SEPARATOR, command::SAVE_PROJECT, command::SAVE_PROJECT_AS, SEPARATOR, command::CLEAN_UP_PROJECT, SEPARATOR, command::QUIT}},
		{"Edit", {command::UNDO, command::REDO, SEPARATOR, command::DELETE_SELECTION}},
		{"View", {command::TOGGLE_MIXER}},
		{"Transport", {command::PLAY_STOP, command::RECORD, command::RETURN_TO_START, command::TOGGLE_METRONOME}},
		{"Track", {command::NEW_TRACK, SEPARATOR, command::ARM_TRACKS, command::MUTE_TRACKS, command::SOLO_TRACKS}},
		{"Settings", {command::AUDIO_SETTINGS, command::INTERFACE_SETTINGS}},
		{"Help", {command::ABOUT}},
	};
}

juce::String describe_shortcut(const juce::KeyPress& key)
{
	if (!key.isValid())
		return {};

	const auto modifiers = key.getModifiers();
	juce::String text;

	if (modifiers.isCtrlDown())
		text << "Ctrl+";

	if (modifiers.isAltDown())
		text << "Alt+";

	if (modifiers.isShiftDown())
		text << "Shift+";

	return text + describe_key(key.getKeyCode());
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
		else if (item == RECENT_MENU)
			popup.addItem(recent_menu());
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

juce::PopupMenu::Item MainMenu::recent_menu()
{
	shown_recent_files = recent_files ? recent_files() : juce::Array<juce::File>();
	juce::PopupMenu submenu;

	for (int index = 0; index < shown_recent_files.size(); ++index)
		submenu.addItem(FIRST_RECENT_ITEM + index, describe_recent(shown_recent_files[index]));

	juce::PopupMenu::Item item("Open Recent");
	item.subMenu = std::make_unique<juce::PopupMenu>(std::move(submenu));
	item.isEnabled = !shown_recent_files.isEmpty() && (!can_open_recent || can_open_recent());
	return item;
}

void MainMenu::menuItemSelected(int item_id, int)
{
	const auto index = item_id - FIRST_RECENT_ITEM;

	if (index >= 0 && index < shown_recent_files.size() && on_open_recent)
		on_open_recent(shown_recent_files[index]);
}
