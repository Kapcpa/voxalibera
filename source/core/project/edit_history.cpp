#include "core/project/edit_history.h"

namespace
{
	double now_in_seconds()
	{
		return juce::Time::getMillisecondCounterHiRes() / 1000.0;
	}

	void revert_all(const Transaction& transaction, Project& project)
	{
		for (auto operation = transaction.operations.rbegin(); operation != transaction.operations.rend(); ++operation)
			operation->revert(project);

		++project.revision;
	}

	void transfer_clip(Project& project, ClipId id, TrackId target)
	{
		auto& source_clips = project.find_track_of(id)->clips;
		const auto clip = std::find_if(source_clips.begin(), source_clips.end(), [id](const Clip& existing) { return existing.id == id; });
		project.find_track(target)->clips.push_back(*clip);
		source_clips.erase(clip);
	}

	void apply_all(const Transaction& transaction, Project& project)
	{
		for (const auto& operation : transaction.operations)
			operation.apply(project);

		++project.revision;
	}
}

Edit::Edit(EditHistory& history, const juce::String& name, const juce::String& merge_key) : history(&history), transaction{name, merge_key, 0.0, {}}
{
}

Edit::Edit(Edit&& other) noexcept : history(other.history), transaction(std::move(other.transaction))
{
	other.history = nullptr;
}

Edit::~Edit()
{
	commit();
}

Project& Edit::current_project()
{
	return history->project;
}

void Edit::record(Operation operation)
{
	operation.apply(current_project());
	++current_project().revision;
	transaction.operations.push_back(std::move(operation));
}

TrackId Edit::add_track(Track track, size_t position)
{
	track.id = current_project().allocate_track_id();
	const auto id = track.id;
	position = std::min(position, current_project().tracks.size());

	record({
		[track, position](Project& target) { target.tracks.insert(target.tracks.begin() + (std::ptrdiff_t) position, track); },
		[id](Project& target) { target.tracks.erase(target.tracks.begin() + target.index_of(id)); },
	});

	return id;
}

void Edit::remove_track(TrackId id)
{
	const auto index = current_project().index_of(id);

	if (index == Project::NOT_FOUND)
		return;

	const auto removed = current_project().tracks[(size_t) index];

	record({
		[id](Project& target) { target.tracks.erase(target.tracks.begin() + target.index_of(id)); },
		[removed, index](Project& target) { target.tracks.insert(target.tracks.begin() + index, removed); },
	});
}

ClipId Edit::add_clip(TrackId track_id, Clip clip)
{
	if (current_project().find_track(track_id) == nullptr)
		return {};

	clip.id = current_project().allocate_clip_id();
	const auto id = clip.id;

	record({
		[track_id, clip](Project& target) { target.find_track(track_id)->clips.push_back(clip); },
		[track_id, id](Project& target) { std::erase_if(target.find_track(track_id)->clips, [id](const Clip& existing) { return existing.id == id; }); },
	});

	return id;
}

void Edit::remove_clip(ClipId id)
{
	auto* track = current_project().find_track_of(id);

	if (track == nullptr)
		return;

	const auto track_id = track->id;
	const auto removed = *current_project().find_clip(id);

	record({
		[track_id, id](Project& target) { std::erase_if(target.find_track(track_id)->clips, [id](const Clip& existing) { return existing.id == id; }); },
		[track_id, removed](Project& target) { target.find_track(track_id)->clips.push_back(removed); },
	});
}

void Edit::move_clip(ClipId id, TrackId target)
{
	const auto* source = current_project().find_track_of(id);

	if (source == nullptr || source->id == target || current_project().find_track(target) == nullptr)
		return;

	const auto source_id = source->id;
	record({[id, target](Project& project) { transfer_clip(project, id, target); }, [id, source_id](Project& project) { transfer_clip(project, id, source_id); }});
}

void Edit::commit()
{
	if (history == nullptr || transaction.operations.empty())
		return;

	transaction.finished_at = now_in_seconds();
	history->commit(std::move(transaction));
	history = nullptr;
}

EditHistory::EditHistory(Project& project) : project(project)
{
}

Edit EditHistory::begin(const juce::String& name, const juce::String& merge_key)
{
	return Edit(*this, name, merge_key);
}

bool EditHistory::merges_with_last(const Transaction& transaction) const
{
	return !done.empty() && transaction.merge_key.isNotEmpty() && done.back().merge_key == transaction.merge_key && transaction.finished_at - done.back().finished_at < MERGE_SECONDS;
}

void EditHistory::commit(Transaction transaction)
{
	undone.clear();

	if (merges_with_last(transaction)) {
		auto& last = done.back();
		std::move(transaction.operations.begin(), transaction.operations.end(), std::back_inserter(last.operations));
		last.finished_at = transaction.finished_at;
		return;
	}

	done.push_back(std::move(transaction));

	if (done.size() > MAX_STEPS)
		done.erase(done.begin());
}

juce::String EditHistory::undo_name() const
{
	return can_undo() ? done.back().name : juce::String();
}

juce::String EditHistory::redo_name() const
{
	return can_redo() ? undone.back().name : juce::String();
}

void EditHistory::undo()
{
	if (!can_undo())
		return;

	revert_all(done.back(), project);
	undone.push_back(std::move(done.back()));
	done.pop_back();
}

void EditHistory::redo()
{
	if (!can_redo())
		return;

	apply_all(undone.back(), project);
	done.push_back(std::move(undone.back()));
	undone.pop_back();
}
