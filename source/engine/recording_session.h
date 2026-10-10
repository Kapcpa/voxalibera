#pragma once

#include "engine/lock_free.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

struct RecordingTake
{
	static constexpr int SAMPLES_PER_PEAK = 256;
	static constexpr size_t PEAK_QUEUE_CAPACITY = 16384;
	static constexpr int MAX_CHANNELS = 2;

	int first_input = 0;
	int channels = 1;
	std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> writer;
	std::atomic<std::int64_t> written{0};
	std::atomic<int> dropped_blocks{0};
	LockFreeQueue<float, PEAK_QUEUE_CAPACITY> peaks;

	std::array<float, MAX_CHANNELS> pending_peaks{};
	int pending_peak_samples = 0;
};

class RecordingSession
{
public:
	static constexpr int SILENCE_SAMPLES = 8192;
	static constexpr std::int64_t NOT_STARTED = -1;

	RecordingSession();

	void write(const float* const* inputs, int input_count, int sample_count, std::int64_t block_start);

	std::vector<std::unique_ptr<RecordingTake>> takes;
	std::atomic<std::int64_t> start_sample{NOT_STARTED};

private:
	void write_take(RecordingTake& take, const float* const* inputs, int input_count, int offset, int sample_count);
	void collect_peaks(RecordingTake& take, const float* const* channels, int sample_count);

	std::vector<float> silence;
};
