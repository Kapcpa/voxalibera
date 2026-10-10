#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

#include <cstdint>
#include <memory>

class AudioFile
{
public:
	static std::shared_ptr<const AudioFile> open(const juce::File& file);

	int channels() const { return (int) reader->numChannels; }
	std::int64_t length() const { return reader->lengthInSamples; }
	double sample_rate() const { return reader->sampleRate; }

	void read(float* const* destination, std::int64_t start, int sample_count) const;
	void touch(std::int64_t start, std::int64_t sample_count) const;

private:
	explicit AudioFile(std::unique_ptr<juce::MemoryMappedAudioFormatReader> mapped_reader);

	std::unique_ptr<juce::MemoryMappedAudioFormatReader> reader;
};
