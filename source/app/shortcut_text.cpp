#include "app/shortcut_text.h"

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
