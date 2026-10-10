#pragma once

#include <juce_core/juce_core.h>

#include <cstdint>
#include <optional>

struct TimeSignature
{
	int beats_per_bar = 4;
	int beat_unit = 4;

	bool operator==(const TimeSignature&) const = default;
};

struct Tempo
{
	double quarter_notes_per_minute = 120.0;
	TimeSignature signature;

	bool operator==(const Tempo&) const = default;
};

struct MusicalPosition
{
	std::int64_t bar = 1;
	int beat = 1;
	int tick = 0;

	bool operator==(const MusicalPosition&) const = default;
};

constexpr int TICKS_PER_BEAT = 960;

double samples_per_beat(const Tempo& tempo, double sample_rate);
std::int64_t convert_samples(std::int64_t samples, double from_rate, double to_rate);
std::int64_t beat_start_sample(std::int64_t beat_index, const Tempo& tempo, double sample_rate);
MusicalPosition to_musical_position(std::int64_t sample, const Tempo& tempo, double sample_rate);
std::int64_t to_sample(const MusicalPosition& position, const Tempo& tempo, double sample_rate);
juce::String format_musical_position(const MusicalPosition& position);
juce::String format_clock(std::int64_t sample, double sample_rate);
juce::String format_time_signature(const TimeSignature& signature);
std::optional<TimeSignature> parse_time_signature(const juce::String& text);
