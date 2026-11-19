#pragma once

#include "core/project/edit_history.h"

#include <cstdint>

int lane_count(const Track& track);
bool has_takes(const Track& track);
bool shows_lanes(const Track& track);
void keep_comp(Edit& edit, const Project& project, TrackId track_id);
ClipId split_clip(Edit& edit, const Project& project, ClipId id, std::int64_t position);
void comp_lane(Edit& edit, const Project& project, TrackId track_id, int lane, std::int64_t start, std::int64_t end);
ClipId add_take(Edit& edit, const Project& project, TrackId track_id, Clip clip);
