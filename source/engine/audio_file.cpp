#include "engine/audio_file.h"

namespace
{
	constexpr std::int64_t TOUCH_STRIDE = 4096;
}

AudioFile::AudioFile(std::unique_ptr<juce::MemoryMappedAudioFormatReader> mapped_reader) : reader(std::move(mapped_reader))
{
}

std::shared_ptr<const AudioFile> AudioFile::open(const juce::File& file)
{
	std::unique_ptr<juce::MemoryMappedAudioFormatReader> reader(juce::WavAudioFormat().createMemoryMappedReader(file));

	if (reader == nullptr || !reader->mapEntireFile())
		return nullptr;

	return std::shared_ptr<const AudioFile>(new AudioFile(std::move(reader)));
}

void AudioFile::read(float* const* destination, std::int64_t start, int sample_count) const
{
	reader->read(destination, channels(), start, sample_count);
}

void AudioFile::touch(std::int64_t start, std::int64_t sample_count) const
{
	const auto end = std::min(length(), start + sample_count);

	for (auto sample = std::max<std::int64_t>(0, start); sample < end; sample += TOUCH_STRIDE)
		reader->touchSample(sample);
}
