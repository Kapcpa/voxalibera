#include "core/media/playback_files.h"

namespace
{
	constexpr double SECONDS_AHEAD = 5.0;
	constexpr int MILLISECONDS_BETWEEN_PASSES = 50;
}

std::shared_ptr<const AudioFile> AudioFilePool::get(const juce::File& file, std::int64_t required_length)
{
	auto& entry = open_files[file.getFullPathName()];

	if (entry == nullptr || entry->length() < required_length)
		entry = AudioFile::open(file);

	return entry != nullptr && entry->length() >= required_length ? entry : nullptr;
}

ReadAhead::ReadAhead(const Transport& transport) : transport(transport)
{
}

void ReadAhead::set_regions(std::vector<ReadAheadRegion> new_regions, double sample_rate)
{
	const std::scoped_lock guard(lock);
	regions = std::move(new_regions);
	rate = sample_rate;
}

int ReadAhead::useTimeSlice()
{
	const std::scoped_lock guard(lock);
	const auto from = transport.position();
	const auto to = from + (std::int64_t) (SECONDS_AHEAD * rate);

	for (const auto& region : regions) {
		const auto overlap_start = std::max(from, region.start);
		const auto overlap_end = std::min(to, region.start + region.length);

		if (overlap_start < overlap_end)
			region.file->touch(region.source_offset + overlap_start - region.start, overlap_end - overlap_start);
	}

	return MILLISECONDS_BETWEEN_PASSES;
}
