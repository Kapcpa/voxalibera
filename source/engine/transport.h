#pragma once

#include "engine/lock_free.h"

#include <atomic>
#include <cstdint>

class Transport
{
public:
	struct Loop
	{
		bool enabled = false;
		std::int64_t start = 0;
		std::int64_t end = 0;

		bool is_active() const { return enabled && end > start; }
		std::int64_t length() const { return end - start; }
		bool operator==(const Loop&) const = default;
	};

	struct Block
	{
		bool playing = false;
		bool recording = false;
		std::int64_t start = 0;
		std::int64_t record_from = 0;
		int length = 0;
		Loop loop{};

		bool is_counting_in() const { return recording && start < record_from; }
		bool wraps_ahead() const { return playing && loop.is_active() && start < loop.end; }
	};

	void play();
	void record(std::int64_t count_in_samples = 0);
	void stop();
	void locate(std::int64_t sample);
	void set_loop(Loop loop);

	bool is_playing() const { return playing.load(std::memory_order_acquire); }
	bool is_recording() const { return recording.load(std::memory_order_acquire); }
	std::int64_t position() const { return published_position.load(std::memory_order_acquire); }
	std::int64_t record_from() const { return published_record_from.load(std::memory_order_acquire); }
	Loop loop() const;

	Block advance(int max_samples);

private:
	enum class Action
	{
		play,
		record,
		stop,
		locate,
		set_loop,
	};

	struct Command
	{
		Action action = Action::stop;
		std::int64_t sample = 0;
		Loop loop{};
	};

	static constexpr size_t COMMAND_CAPACITY = 64;

	void apply_pending_commands();
	Loop effective_loop() const;
	void publish();

	LockFreeQueue<Command, COMMAND_CAPACITY> commands;
	std::atomic<bool> playing{false};
	std::atomic<bool> recording{false};
	std::atomic<std::int64_t> published_position{0};
	std::atomic<std::int64_t> published_record_from{0};
	std::atomic<bool> published_loop_enabled{false};
	std::atomic<std::int64_t> published_loop_start{0};
	std::atomic<std::int64_t> published_loop_end{0};
	bool audio_playing = false;
	bool audio_recording = false;
	std::int64_t audio_position = 0;
	std::int64_t audio_record_from = 0;
	Loop audio_loop;
};
