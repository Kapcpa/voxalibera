#pragma once

#include "engine/engine_snapshot.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>

namespace export_format
{
	constexpr int DITHERED_16_BIT = 16;
	constexpr int INTEGER_24_BIT = 24;
	constexpr int FLOAT_32_BIT = 32;
}

struct ExportJob
{
	std::unique_ptr<EngineSnapshot> snapshot;
	juce::File file;
	double sample_rate = 0.0;
	int bit_depth = export_format::INTEGER_24_BIT;
	std::int64_t length = 0;
	std::int64_t tail = 0;
	std::int64_t pre_roll = 0;
};

struct ExportResult
{
	enum class Outcome
	{
		finished,
		failed,
		cancelled,
	};

	Outcome outcome = Outcome::finished;
	juce::File file;
	float peak = 0.0f;
	bool clipped = false;
	juce::String problem;
};

class MixExport : private juce::Thread
{
public:
	MixExport();
	~MixExport() override;

	void start(ExportJob new_job);
	void cancel();
	bool is_running() const { return isThreadRunning() && !done.load(std::memory_order_acquire); }
	double progress() const { return rendered.load(std::memory_order_relaxed); }
	std::optional<ExportResult> take_result();

private:
	void run() override;
	ExportResult render();
	ExportResult failure(const juce::String& problem) const;
	void add_dither(juce::AudioBuffer<float>& buffer, int start, int count);

	ExportJob job;
	ExportResult result;
	juce::Random random;
	std::atomic<double> rendered{0.0};
	std::atomic<bool> done{false};
};
