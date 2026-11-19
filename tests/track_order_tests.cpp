#include "core/editors.h"

#include <catch2/catch_test_macros.hpp>

namespace
{
	struct Fixture
	{
		Fixture()
		{
			for (const auto* name : {"A", "B", "C", "D"}) {
				Track track;
				track.name = name;
				ids.push_back(history.begin("Add Track").add_track(track, project.tracks.size()));
			}
		}

		void select(std::initializer_list<int> positions)
		{
			selection.tracks.clear();

			for (const auto position : positions)
				selection.tracks.push_back(ids[(size_t) position]);
		}

		juce::String order() const
		{
			juce::String result;

			for (const auto& track : project.tracks)
				result << track.name;

			return result;
		}

		Project project;
		EditHistory history{project};
		Selection selection;
		TrackEditor tracks{project, history, selection};
		std::vector<TrackId> ids;
	};
}

TEST_CASE("selected tracks move up together")
{
	Fixture fixture;
	fixture.select({3, 2});
	fixture.tracks.move_selected(-1);

	CHECK(fixture.order() == "ACDB");
}

TEST_CASE("selected tracks don't move past the edge or apart")
{
	Fixture fixture;
	fixture.select({1, 3});
	fixture.tracks.move_selected(1);

	CHECK(fixture.order() == "ABCD");
}

TEST_CASE("moving a track is undone in one step")
{
	Fixture fixture;
	fixture.tracks.move(fixture.ids[0], 2);

	CHECK(fixture.order() == "BCAD");

	fixture.history.undo();

	CHECK(fixture.order() == "ABCD");
}
