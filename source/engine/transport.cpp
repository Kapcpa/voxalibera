#include "engine/transport.h"

void Transport::play()
{
	commands.push({Action::play});
}

void Transport::stop()
{
	commands.push({Action::stop});
}

void Transport::locate(std::int64_t sample)
{
	commands.push({Action::locate, sample});
}

void Transport::apply_pending_commands()
{
	Command command;

	while (commands.pop(command)) {
		switch (command.action) {
			case Action::play:
				audio_playing = true;
				break;
			case Action::stop:
				audio_playing = false;
				break;
			case Action::locate:
				audio_position = command.sample;
				break;
		}
	}
}

Transport::Block Transport::advance(int sample_count)
{
	apply_pending_commands();

	const Block block{audio_playing, audio_position};

	if (audio_playing)
		audio_position += sample_count;

	playing.store(audio_playing, std::memory_order_release);
	published_position.store(audio_position, std::memory_order_release);
	return block;
}
