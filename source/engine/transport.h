#pragma once

#include "engine/lock_free_queue.h"

#include <atomic>
#include <cstdint>

class Transport
{
public:
	struct Block
	{
		bool playing = false;
		std::int64_t start = 0;
	};

	void play();
	void stop();
	void locate(std::int64_t sample);

	bool is_playing() const { return playing.load(std::memory_order_acquire); }
	std::int64_t position() const { return published_position.load(std::memory_order_acquire); }

	Block advance(int sample_count);

private:
	enum class Action
	{
		play,
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
	std::atomic<std::int64_t> published_position{0};
	bool audio_playing = false;
	std::int64_t audio_position = 0;
};
