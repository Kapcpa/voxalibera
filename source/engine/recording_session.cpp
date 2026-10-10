#include "engine/recording_session.h"

#include <algorithm>
#include <array>
#include <cmath>

RecordingSession::RecordingSession() : silence((size_t) SILENCE_SAMPLES, 0.0f)
{
}

void RecordingSession::write(const float* const* inputs, int input_count, int sample_count, std::int64_t block_start, std::int64_t record_from)
{
	const auto first = (int) std::clamp<std::int64_t>(record_from - block_start, 0, sample_count);

	if (first == sample_count)
		return;

	auto expected = NOT_STARTED;
	start_sample.compare_exchange_strong(expected, block_start + first, std::memory_order_acq_rel);

	for (auto& take : takes)
		for (int offset = first; offset < sample_count; offset += SILENCE_SAMPLES)
			write_take(*take, inputs, input_count, offset, std::min(SILENCE_SAMPLES, sample_count - offset));
}

void RecordingSession::write_take(RecordingTake& take, const float* const* inputs, int input_count, int offset, int sample_count)
{
	std::array<const float*, RecordingTake::MAX_CHANNELS> sources{};

	for (int channel = 0; channel < take.channels; ++channel) {
		const auto input = take.first_input + channel;
		sources[(size_t) channel] = input < input_count ? inputs[input] + offset : silence.data();
	}

	if (!take.writer->write(sources.data(), sample_count))
		take.dropped_blocks.fetch_add(1, std::memory_order_relaxed);

	take.written.fetch_add(sample_count, std::memory_order_release);
	collect_peaks(take, sources.data(), sample_count);
}

void RecordingSession::collect_peaks(RecordingTake& take, const float* const* channels, int sample_count)
{
	for (int sample = 0; sample < sample_count; ++sample) {
		for (int channel = 0; channel < take.channels; ++channel)
			take.pending_peaks[(size_t) channel] = std::max(take.pending_peaks[(size_t) channel], std::abs(channels[channel][sample]));

		if (++take.pending_peak_samples < RecordingTake::SAMPLES_PER_PEAK)
			continue;

		for (int channel = 0; channel < take.channels; ++channel) {
			take.peaks.push(take.pending_peaks[(size_t) channel]);
			take.pending_peaks[(size_t) channel] = 0.0f;
		}

		take.pending_peak_samples = 0;
	}
}
