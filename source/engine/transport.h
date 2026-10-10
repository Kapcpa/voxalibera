#pragma once

#include "engine/lock_free.h"

#include <atomic>
#include <cstdint>

class Transport
{
public:
	struct Block
	{
		bool playing = false;
		bool recording = false;
		std::int64_t start = 0;
		std::int64_t record_from = 0;

		bool is_counting_in() const { return recording && start < record_from; }
	};

	void play();
	void record(std::int64_t count_in_samples = 0);
	void stop();
	void locate(std::int64_t sample);

	bool is_playing() const { return playing.load(std::memory_order_acquire); }
	bool is_recording() const { return recording.load(std::memory_order_acquire); }
	std::int64_t position() const { return published_position.load(std::memory_order_acquire); }
	std::int64_t record_from() const { return published_record_from.load(std::memory_order_acquire); }

	Block advance(int sample_count);

private:
	enum class Action
	{
		play,
		record,
		stop,
		locate,
	};

	struct Command
	{
		Action action = Action::stop;
		std::int64_t sample = 0;
	};

	static constexpr size_t COMMAND_CAPACITY = 64;

	void apply_pending_commands();

	LockFreeQueue<Command, COMMAND_CAPACITY> commands;
	std::atomic<bool> playing{false};
	std::atomic<bool> recording{false};
	std::atomic<std::int64_t> published_position{0};
	std::atomic<std::int64_t> published_record_from{0};
	bool audio_playing = false;
	bool audio_recording = false;
	std::int64_t audio_position = 0;
	std::int64_t audio_record_from = 0;
};
