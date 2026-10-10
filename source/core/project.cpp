#include "core/project.h"

#include <algorithm>

void Project::set_tempo(double quarter_notes_per_minute)
{
	tempo.quarter_notes_per_minute = std::clamp(quarter_notes_per_minute, MIN_TEMPO, MAX_TEMPO);
	++revision;
}

void Project::set_time_signature(TimeSignature signature)
{
	tempo.signature = signature;
	++revision;
}

void Project::set_metronome(bool enabled)
{
	metronome = enabled;
	++revision;
}
