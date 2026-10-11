#include "core/media/playback_files.h"

#include "engine/resampler.h"

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
	const auto [current, current_rate] = copy_regions();
	const auto from = transport.position();
	touch(current, from, from + (std::int64_t) (SECONDS_AHEAD * current_rate));
	return MILLISECONDS_BETWEEN_PASSES;
}

void ReadAhead::load_now(std::int64_t from, std::int64_t length)
{
	touch(copy_regions().first, from, from + length);
}

std::pair<std::vector<ReadAheadRegion>, double> ReadAhead::copy_regions()
{
	const std::scoped_lock guard(lock);
	return {regions, rate};
}

void ReadAhead::touch(const std::vector<ReadAheadRegion>& targets, std::int64_t from, std::int64_t to)
{
	for (const auto& region : targets) {
		const auto overlap_start = std::max(from, region.start);
		const auto overlap_end = std::min(to, region.start + region.length);

		if (overlap_start < overlap_end)
			region.file->touch((std::int64_t) (region.file_start + (double) (overlap_start - region.start) * region.file_step), (std::int64_t) ((double) (overlap_end - overlap_start) * region.file_step) + ResamplingKernel::TAPS);
	}
}
