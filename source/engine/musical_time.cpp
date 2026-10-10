#include "engine/musical_time.h"

#include <cmath>

namespace
{
	constexpr double SECONDS_PER_MINUTE = 60.0;
	constexpr double QUARTER_NOTE_UNIT = 4.0;
	constexpr int SECONDS_PER_CLOCK_MINUTE = 60;
	constexpr int MILLISECONDS_PER_SECOND = 1000;
	constexpr int MAX_BEATS_PER_BAR = 32;
	const juce::Array<int> BEAT_UNITS{1, 2, 4, 8, 16, 32};
}

double samples_per_beat(const Tempo& tempo, double sample_rate)
{
	const auto quarter_notes_per_beat = QUARTER_NOTE_UNIT / tempo.signature.beat_unit;
	return sample_rate * SECONDS_PER_MINUTE / tempo.quarter_notes_per_minute * quarter_notes_per_beat;
}

std::int64_t beat_start_sample(std::int64_t beat_index, const Tempo& tempo, double sample_rate)
{
	return std::llround((double) beat_index * samples_per_beat(tempo, sample_rate));
}

MusicalPosition to_musical_position(std::int64_t sample, const Tempo& tempo, double sample_rate)
{
	const auto beats = (double) sample / samples_per_beat(tempo, sample_rate);
	const auto whole_beats = (std::int64_t) std::floor(beats);
	const auto tick = std::min(TICKS_PER_BEAT - 1, (int) std::floor((beats - (double) whole_beats) * TICKS_PER_BEAT));

	return {
		whole_beats / tempo.signature.beats_per_bar + 1,
		(int) (whole_beats % tempo.signature.beats_per_bar) + 1,
		tick,
	};
}

std::int64_t to_sample(const MusicalPosition& position, const Tempo& tempo, double sample_rate)
{
	const auto beats = (double) (position.bar - 1) * tempo.signature.beats_per_bar + (position.beat - 1) + (double) position.tick / TICKS_PER_BEAT;
	return std::llround(beats * samples_per_beat(tempo, sample_rate));
}

juce::String format_musical_position(const MusicalPosition& position)
{
	return juce::String(position.bar) + "." + juce::String(position.beat) + "." + juce::String(position.tick).paddedLeft('0', 3);
}

juce::String format_clock(std::int64_t sample, double sample_rate)
{
	const auto milliseconds = sample_rate > 0.0 ? (std::int64_t) ((double) sample * MILLISECONDS_PER_SECOND / sample_rate) : 0;
	const auto seconds = milliseconds / MILLISECONDS_PER_SECOND;
	const auto minutes = seconds / SECONDS_PER_CLOCK_MINUTE;

	return juce::String(minutes).paddedLeft('0', 2) + ":" + juce::String(seconds % SECONDS_PER_CLOCK_MINUTE).paddedLeft('0', 2) + "." + juce::String(milliseconds % MILLISECONDS_PER_SECOND).paddedLeft('0', 3);
}

juce::String format_time_signature(const TimeSignature& signature)
{
	return juce::String(signature.beats_per_bar) + "/" + juce::String(signature.beat_unit);
}

std::optional<TimeSignature> parse_time_signature(const juce::String& text)
{
	const auto parts = juce::StringArray::fromTokens(text.removeCharacters(" "), "/", {});

	if (parts.size() != 2 || !parts[0].containsOnly("0123456789") || !parts[1].containsOnly("0123456789"))
		return std::nullopt;

	const TimeSignature signature{parts[0].getIntValue(), parts[1].getIntValue()};

	if (signature.beats_per_bar < 1 || signature.beats_per_bar > MAX_BEATS_PER_BAR || !BEAT_UNITS.contains(signature.beat_unit))
		return std::nullopt;

	return signature;
}
