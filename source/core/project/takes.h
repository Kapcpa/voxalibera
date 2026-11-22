#pragma once

#include "core/project/edit_history.h"

#include <cstdint>
#include <vector>

struct TrackRange
{
	TrackId track;
	std::int64_t start = 0;
	std::int64_t end = 0;
};

int lane_count(const Track& track);
bool has_takes(const Track& track);
bool shows_lanes(const Track& track);
void keep_comp(Edit& edit, const Project& project, TrackId track_id);
ClipId split_clip(Edit& edit, const Project& project, ClipId id, std::int64_t position);
void comp_lane(Edit& edit, const Project& project, TrackId track_id, int lane, std::int64_t start, std::int64_t end);
ClipId add_take(Edit& edit, const Project& project, TrackId track_id, Clip clip);
std::vector<TrackRange> comp_ranges(const Project& project, const std::vector<ClipId>& clips);
std::vector<Clip> takes_in_range(const Track& track, std::int64_t start, std::int64_t end);
void remove_range(Edit& edit, const Project& project, TrackId track_id, std::int64_t start, std::int64_t end);
std::vector<ClipId> add_comp(Edit& edit, const Project& project, TrackId track_id, std::vector<Clip> clips);
void compact_lanes(Edit& edit, const Project& project, TrackId track_id);
