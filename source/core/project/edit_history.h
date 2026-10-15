#pragma once

#include "core/project/model.h"

#include <functional>
#include <vector>

struct Operation
{
	std::function<void(Project&)> apply;
	std::function<void(Project&)> revert;
};

struct Transaction
{
	juce::String name;
	juce::String merge_key;
	double finished_at = 0.0;
	std::vector<Operation> operations;
};

class EditHistory;

class Edit
{
public:
	Edit(EditHistory& history, const juce::String& name, const juce::String& merge_key);
	Edit(Edit&& other) noexcept;
	~Edit();

	template <typename Value>
	void set(Value Project::* field, Value value)
	{
		auto& project = current_project();
		const auto old_value = project.*field;
		record({[field, value](Project& target) { target.*field = value; }, [field, old_value](Project& target) { target.*field = old_value; }});
	}

	template <typename Value>
	void set(TrackId id, Value Track::* field, Value value)
	{
		const auto* track = current_project().find_track(id);

		if (track == nullptr)
			return;

		const auto old_value = track->*field;
		record({[id, field, value](Project& target) { target.find_track(id)->*field = value; }, [id, field, old_value](Project& target) { target.find_track(id)->*field = old_value; }});
	}

	template <typename Value>
	void set(ClipId id, Value Clip::* field, Value value)
	{
		const auto* clip = current_project().find_clip(id);

		if (clip == nullptr)
			return;

		const auto old_value = clip->*field;
		record({[id, field, value](Project& target) { target.find_clip(id)->*field = value; }, [id, field, old_value](Project& target) { target.find_clip(id)->*field = old_value; }});
	}

	template <typename Value>
	void set(PluginId id, Value Plugin::* field, Value value)
	{
		const auto* plugin = current_project().find_plugin(id);

		if (plugin == nullptr)
			return;

		const auto old_value = plugin->*field;
		record({[id, field, value](Project& target) { target.find_plugin(id)->*field = value; }, [id, field, old_value](Project& target) { target.find_plugin(id)->*field = old_value; }});
	}

	TrackId add_track(Track track, size_t position);
	void remove_track(TrackId id);
	ClipId add_clip(TrackId track_id, Clip clip);
	void remove_clip(ClipId id);
	void move_clip(ClipId id, TrackId target);
	PluginId add_plugin(TrackId track_id, Plugin plugin, size_t position);
	void remove_plugin(PluginId id);
	void move_plugin(PluginId id, TrackId target, size_t position);
	void commit();

private:
	Project& current_project();
	void record(Operation operation);

	EditHistory* history;
	Transaction transaction;
};

class EditHistory
{
public:
	static constexpr size_t MAX_STEPS = 500;
	static constexpr double MERGE_SECONDS = 1.5;

	explicit EditHistory(Project& project);

	Edit begin(const juce::String& name, const juce::String& merge_key = {});
	bool can_undo() const { return !done.empty(); }
	bool can_redo() const { return !undone.empty(); }
	juce::String undo_name() const;
	juce::String redo_name() const;
	void undo();
	void redo();
	void clear();

	template <typename Value>
	void set_without_undo(Value Project::* field, Value value)
	{
		project.*field = value;
		++project.revision;
	}

	void mark_changed_without_undo() { ++project.revision; }

private:
	friend class Edit;

	void commit(Transaction transaction);
	bool merges_with_last(const Transaction& transaction) const;

	Project& project;
	std::vector<Transaction> done;
	std::vector<Transaction> undone;
};
