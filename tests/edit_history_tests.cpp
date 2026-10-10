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
