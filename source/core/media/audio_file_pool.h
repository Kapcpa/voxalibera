#pragma once

#include "engine/audio_file.h"

#include <juce_core/juce_core.h>

#include <cstdint>
#include <map>
#include <memory>

class AudioFilePool
{
public:
	std::shared_ptr<const AudioFile> get(const juce::File& file, std::int64_t required_length);

private:
	std::map<juce::String, std::shared_ptr<const AudioFile>> open_files;
};
