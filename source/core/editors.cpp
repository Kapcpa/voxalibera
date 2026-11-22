#include "core/editors.h"

#include "core/clipboard.h"
#include "core/project/model.h"
#include "core/project/takes.h"

#include <algorithm>
#include <iterator>
#include <limits>

namespace
{
	constexpr auto DEFAULT_NAME = "Audio";
	constexpr float MIN_VOLUME_DB = -60.0f;
	constexpr float MAX_VOLUME_DB = 6.0f;
	constexpr float MIN_GAIN_DB = -48.0f;
	constexpr float MAX_GAIN_DB = 24.0f;

	std::int64_t min_clip_length(const Project& project)
	{
		return (std::int64_t) samples_per_beat(project.tempo, project.sample_rate);
	}
}

TrackEditor::TrackEditor(Project& project, EditHistory& edits, Selection& selection) : project(project), edits(edits), selection(selection)
{
}

juce::String TrackEditor::next_default_name() const
{
	return juce::String(DEFAULT_NAME) + " " + juce::String(project.tracks.size() + 1);
}

size_t TrackEditor::insert_position() const
{
	int position = Project::NOT_FOUND;

	for (const auto id : selection.tracks)
		position = std::max(position, project.index_of(id));

	return position == Project::NOT_FOUND ? project.tracks.size() : (size_t) position + 1;
}

TrackId TrackEditor::add(const TrackSettings& settings)
{
	auto edit = edits.begin("Add Track");
	return add(edit, settings);
}

TrackId TrackEditor::add(Edit& edit, const TrackSettings& settings)
{
	Track track;
	track.name = settings.name.isNotEmpty() ? settings.name : next_default_name();
	track.channels = settings.channels;
	track.input = settings.input;
	track.colour = TRACK_COLOURS[(size_t) project.next_id % TRACK_COLOURS.size()].argb;

	const auto id = edit.add_track(track, insert_position());
	selection.select_only(id);
	return id;
}

void TrackEditor::change_settings(TrackId id, const TrackSettings& settings)
{
	const auto* track = project.find_track(id);

	if (track == nullptr)
		return;

	const auto name = settings.name.trim().isNotEmpty() ? settings.name.trim() : track->name;

	if (name == track->name && settings.channels == track->channels && settings.input == track->input)
		return;

	auto edit = edits.begin("Track Settings");
	edit.set(id, &Track::name, name);
	edit.set(id, &Track::channels, settings.channels);
	edit.set(id, &Track::input, settings.input);
}

void TrackEditor::remove_selected()
{
	if (selection.tracks.empty())
		return;

	auto edit = edits.begin(selection.tracks.size() == 1 ? "Delete Track" : "Delete Tracks");

	for (const auto id : selection.tracks)
		edit.remove_track(id);

	selection.clear();
}

void TrackEditor::rename(TrackId id, const juce::String& name)
{
	if (name.trim().isEmpty())
		return;

	edits.begin("Rename Track").set(id, &Track::name, name.trim());
}

void TrackEditor::move_selected(int direction)
{
	if (direction != 1 && direction != -1)
		return;

	auto sorted_tracks(selection.tracks);

	std::erase_if(sorted_tracks, [this](TrackId id) { return project.index_of(id) == Project::NOT_FOUND; });

	if (sorted_tracks.empty())
		return;

	std::sort(sorted_tracks.begin(), sorted_tracks.end(), [this](const TrackId& a, const TrackId& b) { return project.index_of(a) < project.index_of(b); });

	if (direction == 1)
		std::reverse(sorted_tracks.begin(), sorted_tracks.end());

	const auto leading = project.index_of(sorted_tracks.front());
	const auto edge = direction == 1 ? (int) project.tracks.size() - 1 : 0;

	if (leading == edge)
		return;

	auto edit = edits.begin(selection.tracks.size() == 1 ? "Move Track" : "Move Tracks", "move-tracks");

	for (auto id : sorted_tracks)
		edit.move_track(id, (size_t) (project.index_of(id) + direction));
}

void TrackEditor::move(TrackId id, size_t position)
{
	edits.begin("Move Track").move_track(id, position);
}

void TrackEditor::set_colour(TrackId id, std::uint32_t colour)
{
	edits.begin("Track Colour").set(id, &Track::colour, colour);
}

void TrackEditor::set_volume(TrackId id, float volume_db)
{
	edits.begin("Track Volume", "volume:" + juce::String(id.value)).set(id, &Track::volume_db, std::clamp(volume_db, MIN_VOLUME_DB, MAX_VOLUME_DB));
}

void TrackEditor::set_pan(TrackId id, float pan)
{
	edits.begin("Track Pan", "pan:" + juce::String(id.value)).set(id, &Track::pan, std::clamp(pan, -1.0f, 1.0f));
}

void TrackEditor::set_master_volume(float volume_db)
{
	edits.begin("Master Volume", "master-volume").set(&Project::master_volume_db, std::clamp(volume_db, MIN_VOLUME_DB, MAX_VOLUME_DB));
}

void TrackEditor::toggle(TrackId id, bool Track::* flag, const juce::String& action_name)
{
	if (const auto* track = project.find_track(id))
		edits.begin(action_name).set(id, flag, !(track->*flag));
}

void TrackEditor::toggle_selected(bool Track::* flag, const juce::String& action_name)
{
	if (selection.tracks.empty())
		return;

	const auto* first = project.find_track(selection.tracks.front());
	const auto value = first != nullptr && !(first->*flag);
	auto edit = edits.begin(action_name);

	for (const auto id : selection.tracks)
		edit.set(id, flag, value);
}

PluginId TrackEditor::add_plugin(TrackId id, const Plugin& plugin)
{
	const auto* track = project.find_track(id);
	return track == nullptr ? PluginId{} : insert_plugin(id, plugin, track->plugins.size());
}

PluginId TrackEditor::insert_plugin(TrackId id, const Plugin& plugin, size_t position)
{
	return edits.begin("Add " + plugin.name).add_plugin(id, plugin, position);
}

void TrackEditor::remove_plugin(PluginId id)
{
	const auto* plugin = project.find_plugin(id);

	if (plugin == nullptr)
		return;

	edits.begin("Remove " + plugin->name).remove_plugin(id);

	if (selection.plugin == id)
		selection.clear_chain();
}

void TrackEditor::move_plugin(PluginId id, TrackId target, size_t position)
{
	const auto* plugin = project.find_plugin(id);
	const auto* source = project.find_track_of(id);

	if (plugin == nullptr || project.find_track(target) == nullptr)
		return;

	const auto current_position = (size_t) (plugin - source->plugins.data());
	const auto last_position = source->id == target ? source->plugins.size() - 1 : project.find_track(target)->plugins.size();
	position = std::min(position, last_position);

	if (source->id == target && position == current_position)
		return;

	edits.begin("Move " + plugin->name).move_plugin(id, target, position);
}

void TrackEditor::toggle_plugin_bypass(PluginId id)
{
	if (const auto* plugin = project.find_plugin(id))
		edits.begin("Bypass " + plugin->name).set(id, &Plugin::bypass, !plugin->bypass);
}

ClipEditor::ClipEditor(Project& project, EditHistory& edits, Selection& selection) : project(project), edits(edits), selection(selection)
{
}

ClipMove ClipEditor::limit_move(ClipMove move) const
{
	auto earliest_start = std::numeric_limits<std::int64_t>::max();
	auto lowest_track = std::numeric_limits<int>::max();
	auto highest_track = 0;

	for (const auto id : selection.clips) {
		const auto* clip = project.find_clip(id);
		const auto* track = project.find_track_of(id);

		if (clip == nullptr || track == nullptr)
			continue;

		const auto index = project.index_of(track->id);
		earliest_start = std::min(earliest_start, clip->start);
		lowest_track = std::min(lowest_track, index);
		highest_track = std::max(highest_track, index);
	}

	if (lowest_track == std::numeric_limits<int>::max())
		return {};

	move.delta = std::max(move.delta, -earliest_start);
	move.track_offset = std::clamp(move.track_offset, -lowest_track, (int) project.tracks.size() - 1 - highest_track);
	return move;
}

void ClipEditor::move_selected(ClipMove move)
{
	move = limit_move(move);

	if (move.is_empty())
		return;

	auto edit = edits.begin(selection.clips.size() == 1 ? "Move Clip" : "Move Clips");

	for (const auto id : selection.clips) {
		const auto* clip = project.find_clip(id);
		const auto track_index = project.index_of(project.find_track_of(id)->id);
		edit.set(id, &Clip::start, clip->start + move.delta);
		edit.move_clip(id, project.tracks[(size_t) (track_index + move.track_offset)].id);
	}
}

void ClipEditor::remove_selected()
{
	if (selection.clips.empty())
		return;

	const auto ranges = comp_ranges(project, selection.clips);
	std::vector<ClipId> plain_clips;

	for (const auto id : selection.clips)
		if (const auto* track = project.find_track_of(id); track != nullptr && !has_takes(*track))
			plain_clips.push_back(id);

	auto edit = edits.begin(selection.clips.size() == 1 ? "Delete Clip" : "Delete Clips");

	for (const auto id : plain_clips)
		edit.remove_clip(id);

	for (const auto& range : ranges)
		remove_range(edit, project, range.track, range.start, range.end);

	edit.commit();
	selection.forget_missing(project);
}

void ClipEditor::split_selected(std::int64_t position, double device_rate)
{
	if (selection.clips.empty())
		return;

	auto edit = edits.begin(selection.clips.size() == 1 ? "Split Clip" : "Split Clips");

	position = convert_samples(position, device_rate, timeline_rate(project, device_rate));

	std::vector<ClipId> right_clips;

	for (const auto id : selection.clips)
		if (const auto right_clip_id = split_clip(edit, project, id, position); right_clip_id.is_valid())
			right_clips.push_back(right_clip_id);

	if (right_clips.empty())
		return;

	selection.clips.insert(selection.clips.end(), right_clips.begin(), right_clips.end());
	++selection.revision;
}

void ClipEditor::comp(TrackId track, int lane, std::int64_t start, std::int64_t end)
{
	auto edit = edits.begin("Comp Takes");
	comp_lane(edit, project, track, lane, std::min(start, end), std::max(start, end));
	edit.commit();
	forget_inactive();
}

void ClipEditor::choose_take(ClipId id)
{
	const auto* clip = project.find_clip(id);
	const auto* track = project.find_track_of(id);

	if (clip == nullptr || track == nullptr)
		return;

	comp(track->id, clip->lane, clip->start, clip->end());
}

void ClipEditor::keep_comp(TrackId track)
{
	auto edit = edits.begin("Keep Comp");
	::keep_comp(edit, project, track);
	edit.commit();
	selection.forget_missing(project);
}

void ClipEditor::forget_inactive()
{
	const auto removed = std::erase_if(selection.clips, [this](ClipId id) {
		const auto* clip = project.find_clip(id);
		return clip == nullptr || !clip->active;
	});

	if (removed > 0)
		++selection.revision;
}

void ClipEditor::set_gain(ClipId id, float gain_db)
{
	edits.begin("Clip Gain", "gain:" + juce::String(id.value)).set(id, &Clip::gain_db, std::clamp(gain_db, MIN_GAIN_DB, MAX_GAIN_DB));
}

void ClipEditor::set_fade_in(ClipId id, std::int64_t fade_in)
{
	const auto* clip = project.find_clip(id);
	edits.begin("Clip Fade In", "fade:" + juce::String(id.value)).set(id, &Clip::fade_in, std::clamp<std::int64_t>(fade_in, 0, clip->length - clip->fade_out));
}

void ClipEditor::set_fade_out(ClipId id, std::int64_t fade_out)
{
	const auto* clip = project.find_clip(id);
	edits.begin("Clip Fade Out", "fade:" + juce::String(id.value)).set(id, &Clip::fade_out, std::clamp<std::int64_t>(fade_out, 0, clip->length - clip->fade_in));
}

void ClipEditor::set_trim_left(ClipId id, std::int64_t trim_left)
{
	const auto* clip = project.find_clip(id);
	const auto clip_trim = std::clamp<std::int64_t>(trim_left, -clip->source_offset, std::max<std::int64_t>(0, clip->length - min_clip_length(project)));
	const auto clip_length = clip->length - clip_trim;
	const auto fade_out = std::min(clip->fade_out, clip_length);
	const auto fade_in = std::min(clip->fade_in, clip_length - fade_out);

	auto edit = edits.begin("Clip Trim Left", "trim:" + juce::String(id.value));
	edit.set(id, &Clip::start, clip->start + clip_trim);
	edit.set(id, &Clip::source_offset, clip->source_offset + clip_trim);
	edit.set(id, &Clip::length, clip_length);
	edit.set(id, &Clip::fade_in, fade_in);
	edit.set(id, &Clip::fade_out, fade_out);
}

void ClipEditor::set_trim_right(ClipId id, std::int64_t trim_right)
{
	const auto* clip = project.find_clip(id);
	const auto source_left = clip->source_length - clip->source_offset - clip->length;
	const auto clip_trim = std::clamp<std::int64_t>(trim_right, -std::max<std::int64_t>(0, source_left), std::max<std::int64_t>(0, clip->length - min_clip_length(project)));
	const auto clip_length = clip->length - clip_trim;
	const auto fade_in = std::min(clip->fade_in, clip_length);
	const auto fade_out = std::min(clip->fade_out, clip_length - fade_in);

	auto edit = edits.begin("Clip Trim Right", "trim:" + juce::String(id.value));
	edit.set(id, &Clip::length, clip_length);
	edit.set(id, &Clip::fade_in, fade_in);
	edit.set(id, &Clip::fade_out, fade_out);
}

void ClipEditor::paste_clips(const Clipboard& clipboard, std::int64_t position, double device_rate)
{
	if (clipboard.clips.empty())
		return;

	position = convert_samples(position, device_rate, timeline_rate(project, device_rate));

	auto edit = edits.begin(clipboard.clips.size() == 1 ? "Paste Clip" : "Paste Clips");
	const auto pasted_clips = add_copies(edit, clipboard.clips, position);
	edit.commit();
	select_only(pasted_clips);
}

void ClipEditor::duplicate_selected()
{
	const auto copies = copies_of(selection, project);

	if (copies.empty())
		return;

	const auto earliest_start = std::ranges::min(copies, {}, [](const CopiedClip& entry) { return entry.clip.start; }).clip.start;
	const auto latest_end = std::ranges::max(copies, {}, [](const CopiedClip& entry) { return entry.clip.end(); }).clip.end();

	auto edit = edits.begin(selection.clips.size() == 1 ? "Duplicate Clip" : "Duplicate Clips");
	const auto duplicated_clips = add_copies(edit, copies, latest_end - earliest_start);
	edit.commit();
	select_only(duplicated_clips);
}

std::vector<ClipId> ClipEditor::add_copies(Edit& edit, const std::vector<CopiedClip>& copies, std::int64_t offset)
{
	std::vector<TrackId> tracks;

	for (const auto& entry : copies)
		if (std::ranges::find(tracks, entry.track) == tracks.end())
			tracks.push_back(entry.track);

	std::vector<ClipId> added;

	for (const auto track_id : tracks) {
		const auto* track = project.find_track(track_id);

		if (track == nullptr)
			continue;

		std::vector<Clip> clips;

		for (const auto& entry : copies)
			if (entry.track == track_id)
				clips.push_back(entry.clip);

		for (auto& clip : clips)
			clip.start += offset;

		const auto is_comp = has_takes(*track) || std::ranges::any_of(clips, [](const Clip& clip) { return clip.lane > 0 || !clip.active; });

		if (is_comp) {
			std::ranges::copy(add_comp(edit, project, track_id, clips), std::back_inserter(added));
			continue;
		}

		for (const auto& clip : clips)
			added.push_back(edit.add_clip(track_id, clip));
	}

	return added;
}

void ClipEditor::select_only(std::vector<ClipId> ids)
{
	std::erase_if(ids, [this](ClipId id) {
		const auto* clip = project.find_clip(id);
		return clip == nullptr || !clip->active;
	});

	if (ids.empty())
		return;

	selection.clips = std::move(ids);
	++selection.revision;
}
