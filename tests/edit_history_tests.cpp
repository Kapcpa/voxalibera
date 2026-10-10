#include "core/project/edit_history.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
	Track named(const juce::String& name)
	{
		Track track;
		track.name = name;
		return track;
	}

	juce::StringArray names(const Project& project)
	{
		juce::StringArray result;

		for (const auto& track : project.tracks)
			result.add(track.name);

		return result;
	}
}

TEST_CASE("adding and removing tracks can be undone and redone")
{
	Project project;
	EditHistory history(project);

	const auto guitar = history.begin("Add").add_track(named("Guitar"), 0);
	history.begin("Add").add_track(named("Vocal"), 1);
	history.begin("Remove").remove_track(guitar);
	CHECK(names(project) == juce::StringArray{"Vocal"});

	history.undo();
	CHECK(names(project) == juce::StringArray{"Guitar", "Vocal"});

	history.undo();
	history.undo();
	CHECK(project.tracks.empty());
	CHECK_FALSE(history.can_undo());

	history.redo();
	history.redo();
	CHECK(names(project) == juce::StringArray{"Guitar", "Vocal"});
	CHECK(project.tracks.front().id == guitar);
}

TEST_CASE("changing a field remembers the old value")
{
	Project project;
	EditHistory history(project);
	const auto id = history.begin("Add").add_track(named("Guitar"), 0);

	history.begin("Rename").set(id, &Track::name, juce::String("Lead"));
	history.begin("Tempo").set(&Project::tempo, Tempo{90.0, {3, 4}});
	CHECK(project.find_track(id)->name == "Lead");

	history.undo();
	history.undo();
	CHECK(project.find_track(id)->name == "Guitar");
	CHECK(project.tempo == Tempo{});
}

TEST_CASE("everything inside one edit is a single undo step")
{
	Project project;
	EditHistory history(project);

	{
		auto edit = history.begin("Add two");
		edit.add_track(named("A"), 0);
		edit.add_track(named("B"), 1);
	}

	history.undo();
	CHECK(project.tracks.empty());
}

TEST_CASE("quick edits with the same merge key become one undo step")
{
	Project project;
	EditHistory history(project);
	const auto id = history.begin("Add").add_track(named("Guitar"), 0);

	for (const auto volume : {-1.0f, -2.0f, -3.0f})
		history.begin("Volume", "volume").set(id, &Track::volume_db, volume);

	CHECK(std::lround(project.find_track(id)->volume_db) == -3);
	history.undo();
	CHECK(std::lround(project.find_track(id)->volume_db) == 0);
	CHECK(history.undo_name() == "Add");
}

TEST_CASE("a new edit clears what could be redone")
{
	Project project;
	EditHistory history(project);
	history.begin("Add").add_track(named("A"), 0);
	history.undo();
	history.begin("Add").add_track(named("B"), 0);

	CHECK_FALSE(history.can_redo());
	CHECK(names(project) == juce::StringArray{"B"});
}

TEST_CASE("clips can be added, moved and removed, and undone")
{
	Project project;
	EditHistory history(project);
	const auto track = history.begin("Add").add_track(named("Guitar"), 0);

	Clip take;
	take.file = "audio/guitar-take-1.wav";
	take.length = 1000;
	const auto clip = history.begin("Record").add_clip(track, take);

	history.begin("Move").set(clip, &Clip::start, std::int64_t{48000});
	history.begin("Delete").remove_clip(clip);
	CHECK(project.find_clip(clip) == nullptr);

	history.undo();
	CHECK(project.find_clip(clip)->start == 48000);

	history.undo();
	CHECK(project.find_clip(clip)->start == 0);

	history.undo();
	CHECK(project.find_track(track)->clips.empty());
}

TEST_CASE("a clip moved to another track keeps its id and moves back on undo")
{
	Project project;
	EditHistory history(project);
	const auto guitar = history.begin("Add").add_track(named("Guitar"), 0);
	const auto bass = history.begin("Add").add_track(named("Bass"), 1);
	const auto clip = history.begin("Record").add_clip(guitar, Clip{});

	history.begin("Move").move_clip(clip, bass);
	CHECK(project.find_track_of(clip)->id == bass);

	history.undo();
	CHECK(project.find_track_of(clip)->id == guitar);
}
