#include "app/commands.h"

namespace
{
	const juce::ModifierKeys NO_MODIFIERS;
	const juce::ModifierKeys SHIFT{juce::ModifierKeys::shiftModifier};
	const juce::ModifierKeys CONTROL{juce::ModifierKeys::commandModifier};
	const juce::ModifierKeys CONTROL_SHIFT{juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier};

	const std::vector<CommandDefinition> DEFINITIONS{
		{command::NEW_PROJECT, "New Project", "File", 'n', CONTROL},
		{command::OPEN_PROJECT, "Open Project...", "File", 'o', CONTROL},
		{command::SAVE_PROJECT, "Save", "File", 's', CONTROL},
		{command::SAVE_PROJECT_AS, "Save As...", "File", 's', CONTROL_SHIFT},
		{command::CLEAN_UP_PROJECT, "Clean Up Project...", "File", 0, NO_MODIFIERS},
		{command::QUIT, "Quit", "File", 'q', CONTROL},
		{command::UNDO, "Undo", "Edit", 'z', CONTROL},
		{command::REDO, "Redo", "Edit", 'z', CONTROL_SHIFT},
		{command::TOGGLE_MIXER, "Mixer", "View", 'x', NO_MODIFIERS},
		{command::PLAY_STOP, "Play / Stop", "Transport", juce::KeyPress::spaceKey, NO_MODIFIERS},
		{command::RETURN_TO_START, "Return to Start", "Transport", juce::KeyPress::returnKey, NO_MODIFIERS},
		{command::RECORD, "Record", "Transport", 'r', CONTROL},
		{command::TOGGLE_METRONOME, "Metronome", "Transport", 'k', NO_MODIFIERS},
		{command::TOGGLE_COUNT_IN, "Count-in", "Transport", 'k', SHIFT},
		{command::NEW_TRACK, "New Track...", "Track", 't', CONTROL},
		{command::DELETE_SELECTION, "Delete", "Edit", juce::KeyPress::deleteKey, NO_MODIFIERS},
		{command::ARM_TRACKS, "Arm", "Track", 'r', NO_MODIFIERS},
		{command::MUTE_TRACKS, "Mute", "Track", 'm', NO_MODIFIERS},
		{command::SOLO_TRACKS, "Solo", "Track", 's', NO_MODIFIERS},
		{command::AUDIO_SETTINGS, "Audio...", "Settings", ',', CONTROL},
		{command::INTERFACE_SETTINGS, "Interface...", "Settings", 0, NO_MODIFIERS},
		{command::ABOUT, "About Voxalibera", "Help", 0, NO_MODIFIERS},
	};
}

const std::vector<CommandDefinition>& command_definitions()
{
	return DEFINITIONS;
}

const CommandDefinition* find_command(juce::CommandID id)
{
	for (const auto& definition : DEFINITIONS)
		if (definition.id == id)
			return &definition;

	return nullptr;
}
