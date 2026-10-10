#pragma once

#include "engine/musical_time.h"

#include <juce_core/juce_core.h>

#include <array>
#include <compare>
#include <cstdint>
#include <vector>

template <typename Tag>
struct Id
{
	int value = 0;

	bool is_valid() const { return value > 0; }
	auto operator<=>(const Id&) const = default;
};

using TrackId = Id<struct TrackTag>;
using ClipId = Id<struct ClipTag>;

struct Clip
{
	ClipId id;
	juce::String file;
	std::int64_t start = 0;
	std::int64_t source_offset = 0;
	std::int64_t length = 0;
	float gain_db = 0.0f;
	int lane = 0;

	std::int64_t end() const { return start + length; }
};

struct TrackColour
{
	const char* name;
	std::uint32_t argb;
};

inline constexpr std::array<TrackColour, 8> TRACK_COLOURS{{
	{"Orange", 0xffd9813b},
	{"Blue", 0xff5b8def},
	{"Green", 0xff4fb37a},
	{"Pink", 0xffc75c9a},
	{"Yellow", 0xffd4b84a},
	{"Teal", 0xff58b8c4},
	{"Purple", 0xff9b7be0},
	{"Red", 0xffe0675c},
}};

enum class TrackChannels
{
	mono,
	stereo,
};

struct TrackInput
{
	static constexpr int NONE = -1;

	int first_channel = 0;

	bool is_connected() const { return first_channel != NONE; }
	bool operator==(const TrackInput&) const = default;
};

struct Track
{
	TrackId id;
	juce::String name;
	std::uint32_t colour = 0;
	TrackChannels channels = TrackChannels::mono;
	TrackInput input;
	bool armed = false;
	bool mute = false;
	bool solo = false;
	float volume_db = 0.0f;
	float pan = 0.0f;
	std::vector<Clip> clips;
};

int channel_count(TrackChannels channels);
juce::String describe_channels(TrackChannels channels);
juce::String describe_input(const TrackInput& input, TrackChannels channels);
bool any_solo(const std::vector<Track>& tracks);
bool is_audible(const Track& track, bool solo_active);

struct Project
{
	static constexpr double MIN_TEMPO = 20.0;
	static constexpr double MAX_TEMPO = 300.0;
	static constexpr int NOT_FOUND = -1;

	Track* find_track(TrackId id);
	const Track* find_track(TrackId id) const;
	int index_of(TrackId id) const;
	Clip* find_clip(ClipId id);
	const Clip* find_clip(ClipId id) const;
	Track* find_track_of(ClipId id);
	const Track* find_track_of(ClipId id) const;
	TrackId allocate_track_id();
	ClipId allocate_clip_id();

	juce::File folder;
	Tempo tempo;
	bool metronome = true;
	float master_volume_db = 0.0f;
	std::vector<Track> tracks;
	int next_id = 1;
	int revision = 0;
};
