#include "core/media/audio_file_pool.h"

std::shared_ptr<const AudioFile> AudioFilePool::get(const juce::File& file, std::int64_t required_length)
{
	auto& entry = open_files[file.getFullPathName()];

	if (entry == nullptr || entry->length() < required_length)
		entry = AudioFile::open(file);

	return entry != nullptr && entry->length() >= required_length ? entry : nullptr;
}
