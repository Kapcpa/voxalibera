#pragma once

#include "engine/audio_file.h"
#include "engine/transport.h"

#include <juce_core/juce_core.h>

#include <memory>
#include <mutex>
#include <vector>

struct ReadAheadRegion
{
	std::shared_ptr<const AudioFile> file;
	std::int64_t start = 0;
	std::int64_t source_offset = 0;
	std::int64_t length = 0;
};

class ReadAhead : public juce::TimeSliceClient
{
public:
	explicit ReadAhead(const Transport& transport);

	void set_regions(std::vector<ReadAheadRegion> new_regions, double sample_rate);
	int useTimeSlice() override;

private:
	const Transport& transport;
	std::mutex lock;
	std::vector<ReadAheadRegion> regions;
	double rate = 0.0;
};
