#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace command
{
	constexpr juce::CommandID NEW_PROJECT = 1;
	constexpr juce::CommandID OPEN_PROJECT = 2;
	constexpr juce::CommandID SAVE_PROJECT = 3;
	constexpr juce::CommandID SAVE_PROJECT_AS = 4;
	constexpr juce::CommandID QUIT = 5;
	constexpr juce::CommandID CLEAN_UP_PROJECT = 6;
	constexpr juce::CommandID UNDO = 10;
	constexpr juce::CommandID REDO = 11;
	constexpr juce::CommandID TOGGLE_MIXER = 20;
	constexpr juce::CommandID PLAY_STOP = 25;
	constexpr juce::CommandID RETURN_TO_START = 26;
	constexpr juce::CommandID TOGGLE_METRONOME = 27;
	constexpr juce::CommandID RECORD = 28;
	constexpr juce::CommandID TOGGLE_COUNT_IN = 29;
	constexpr juce::CommandID NEW_TRACK = 60;
	constexpr juce::CommandID DELETE_SELECTION = 61;
	constexpr juce::CommandID ARM_TRACKS = 62;
	constexpr juce::CommandID MUTE_TRACKS = 63;
	constexpr juce::CommandID SOLO_TRACKS = 64;
	constexpr juce::CommandID AUDIO_SETTINGS = 30;
	constexpr juce::CommandID INTERFACE_SETTINGS = 31;
	constexpr juce::CommandID ABOUT = 40;
}

struct CommandDefinition
{
	juce::CommandID id = 0;
	juce::String name;
	juce::String category;
	int key = 0;
	juce::ModifierKeys modifiers;
};

const std::vector<CommandDefinition>& command_definitions();
const CommandDefinition* find_command(juce::CommandID id);
