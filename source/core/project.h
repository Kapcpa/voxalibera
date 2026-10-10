#pragma once

#include "engine/musical_time.h"

struct Project
{
	static constexpr double MIN_TEMPO = 20.0;
	static constexpr double MAX_TEMPO = 300.0;

	void set_tempo(double quarter_notes_per_minute);
	void set_time_signature(TimeSignature signature);
	void set_metronome(bool enabled);

	Tempo tempo;
	bool metronome = true;
	int revision = 0;
};
