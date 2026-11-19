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
		{command::QUIT, "Quit", "File", 'q', CONTROL},
		{command::CLEAN_UP_PROJECT, "Clean Up Project...", "File", 0, NO_MODIFIERS},
		{command::IMPORT_FILE, "Import File...", "File", 0, NO_MODIFIERS},
		{command::EXPORT_MIX, "Export Mix...", "File", 'e', CONTROL_SHIFT},
		{command::COPY, "Copy", "Edit", 'c', CONTROL},
		{command::PASTE, "Paste", "Edit", 'v', CONTROL},
		{command::DUPLICATE, "Duplicate", "Edit", 'd', CONTROL},
		{command::CUT, "Cut", "Edit", 'x', CONTROL},
		{command::UNDO, "Undo", "Edit", 'z', CONTROL},
		{command::REDO, "Redo", "Edit", 'z', CONTROL_SHIFT},
		{command::SPLIT, "Split", "Edit", 'e', CONTROL},
		{command::FINER_GRID, "Finer Grid", "Edit", '1', CONTROL},
		{command::COARSER_GRID, "Coarser Grid", "Edit", '2', CONTROL},
		{command::TRIPLET_GRID, "Triplet Grid", "Edit", '3', CONTROL},
		{command::TOGGLE_SNAP, "Snap to Grid", "Edit", '4', CONTROL},
		{command::ADAPTIVE_GRID, "Adaptive Grid", "Edit", '5', CONTROL},
		{command::TOGGLE_MIXER, "Mixer", "View", 'x', NO_MODIFIERS},
		{command::TOGGLE_PLUGIN_CHAIN, "Plugin Chain", "View", 'v', NO_MODIFIERS},
		{command::PLAY_STOP, "Play / Stop", "Transport", juce::KeyPress::spaceKey, NO_MODIFIERS},
		{command::RETURN_TO_START, "Return to Start", "Transport", juce::KeyPress::returnKey, NO_MODIFIERS},
		{command::RECORD, "Record", "Transport", 'r', CONTROL},
		{command::TOGGLE_LOOP, "Loop", "Transport", 'l', NO_MODIFIERS},
		{command::TOGGLE_METRONOME, "Metronome", "Transport", 'k', NO_MODIFIERS},
		{command::TOGGLE_COUNT_IN, "Count-in", "Transport", 'k', SHIFT},
		{command::NEW_TRACK, "New Track...", "Track", 't', CONTROL},
		{command::TRACK_SETTINGS, "Track Settings...", "Track", 0, NO_MODIFIERS},
		{command::DELETE_SELECTION, "Delete", "Edit", juce::KeyPress::deleteKey, NO_MODIFIERS},
		{command::ARM_TRACKS, "Arm", "Track", 'r', NO_MODIFIERS},
		{command::MUTE_TRACKS, "Mute", "Track", 'm', NO_MODIFIERS},
		{command::SOLO_TRACKS, "Solo", "Track", 's', NO_MODIFIERS},
		{command::BYPASS_TRACK_PLUGINS, "Bypass Effects", "Track", 'b', NO_MODIFIERS},
		{command::AUDIO_SETTINGS, "Audio...", "Settings", ',', CONTROL},
		{command::INTERFACE_SETTINGS, "Interface...", "Settings", 0, NO_MODIFIERS},
		{command::PLUGIN_SETTINGS, "Plugin...", "Settings", 0, NO_MODIFIERS},
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
