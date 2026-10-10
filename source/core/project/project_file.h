#pragma once

#include "core/project/model.h"

#include <juce_core/juce_core.h>

#include <optional>

constexpr int PROJECT_FORMAT = 1;
constexpr auto PROJECT_EXTENSION = ".vxl";

struct LoadedProject
{
	Project project;
	juce::StringArray problems;
	bool newer_format = false;
};

juce::String write_project(const Project& project, const juce::String& app_version);
std::optional<LoadedProject> read_project(const juce::String& text);
