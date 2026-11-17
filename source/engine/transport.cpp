#include "engine/transport.h"

#include <algorithm>

void Transport::play()
{
	commands.push({Action::play});
}

void Transport::record(std::int64_t count_in_samples)
{
	commands.push({Action::record, count_in_samples});
}

void Transport::stop()
{
	commands.push({Action::stop});
}

void Transport::locate(std::int64_t sample)
{
	commands.push({Action::locate, sample});
}

void Transport::set_loop(Loop loop)
{
	commands.push({Action::set_loop, 0, loop});
}

Transport::Loop Transport::loop() const
{
	return {published_loop_enabled.load(std::memory_order_acquire), published_loop_start.load(std::memory_order_acquire), published_loop_end.load(std::memory_order_acquire)};
}

void Transport::apply_pending_commands()
{
	Command command;

	while (commands.pop(command)) {
		switch (command.action) {
			case Action::play:
				audio_playing = true;
				break;
			case Action::record:
				audio_record_from = audio_position;
				audio_position -= audio_playing ? 0 : command.sample;
				audio_playing = true;
				audio_recording = true;
				break;
			case Action::stop:
				audio_playing = false;
				audio_recording = false;
				break;
			case Action::locate:
				audio_position = command.sample;
				break;
			case Action::set_loop:
				audio_loop = command.loop;
				break;
		}
	}
}

Transport::Loop Transport::effective_loop() const
{
	if (audio_recording && audio_record_from >= audio_loop.end)
		return {};

	return audio_loop;
}

Transport::Block Transport::advance(int max_samples)
{
	apply_pending_commands();

	const auto loop = effective_loop();
	const auto wraps = audio_playing && loop.is_active() && audio_position < loop.end && audio_position + max_samples >= loop.end;
	const auto length = wraps ? (int) (loop.end - audio_position) : max_samples;
	const Block block{audio_playing, audio_recording, audio_position, audio_record_from, length, loop};

	if (audio_playing)
		audio_position += length;

	if (wraps) {
		audio_position = loop.start;

		if (audio_recording)
			audio_record_from = std::min(audio_record_from, loop.start);
	}

	publish();
	return block;
}

void Transport::publish()
{
	playing.store(audio_playing, std::memory_order_release);
	recording.store(audio_recording, std::memory_order_release);
	published_position.store(audio_position, std::memory_order_release);
	published_record_from.store(audio_record_from, std::memory_order_release);
	published_loop_enabled.store(audio_loop.enabled, std::memory_order_release);
	published_loop_start.store(audio_loop.start, std::memory_order_release);
	published_loop_end.store(audio_loop.end, std::memory_order_release);
}
