#pragma once

#include "engine/audio_file.h"
#include "engine/transport.h"

#include <juce_core/juce_core.h>

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class AudioFilePool
{
public:
	std::shared_ptr<const AudioFile> get(const juce::File& file, std::int64_t required_length);

private:
	std::map<juce::String, std::shared_ptr<const AudioFile>> open_files;
};

struct ReadAheadRegion
{
	std::shared_ptr<const AudioFile> file;
	std::int64_t start = 0;
	std::int64_t length = 0;
	double file_start = 0.0;
	double file_step = 1.0;
};

class ReadAhead : public juce::TimeSliceClient
{
public:
	explicit ReadAhead(const Transport& transport);

	void set_regions(std::vector<ReadAheadRegion> new_regions, double sample_rate);
	int useTimeSlice() override;
	void load_now(std::int64_t from, std::int64_t length);

private:
	std::pair<std::vector<ReadAheadRegion>, double> copy_regions();
	static void touch(const std::vector<ReadAheadRegion>& targets, std::int64_t from, std::int64_t to);

	const Transport& transport;
	std::mutex lock;
	std::vector<ReadAheadRegion> regions;
	double rate = 0.0;
};
