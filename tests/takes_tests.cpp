#include "core/clipboard.h"
#include "core/editors.h"
#include "core/project/takes.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

namespace
{
	constexpr double SAMPLE_RATE = 1000.0;
	constexpr std::int64_t EDGE_FADE = 5;

	struct Fixture
	{
		Fixture()
		{
			project.sample_rate = SAMPLE_RATE;
			track = history.begin("Add Track").add_track(Track{}, 0);
		}

		ClipId record(std::int64_t start, std::int64_t length, const juce::String& file)
		{
			Clip clip;
			clip.file = file;
			clip.start = start;
			clip.length = length;
			clip.source_length = length;
			auto edit = history.begin("Record");
			return add_take(edit, project, track, clip);
		}

		void comp(int lane, std::int64_t start, std::int64_t end)
		{
			auto edit = history.begin("Comp");
			comp_lane(edit, project, track, lane, start, end);
		}

		std::vector<Clip> active_clips() const
		{
			std::vector<Clip> result;

			for (const auto& clip : project.find_track(track)->clips)
				if (clip.active)
					result.push_back(clip);

			std::sort(result.begin(), result.end(), [](const Clip& left, const Clip& right) { return left.start < right.start; });
			return result;
		}

		void select_active_at(std::int64_t position)
		{
			for (const auto& clip : active_clips())
				if (clip.start <= position && position < clip.end())
					selection.select_only(clip.id);
		}

		bool active_clips_overlap() const
		{
			const auto active = active_clips();

			for (size_t index = 1; index < active.size(); ++index)
				if (active[index].start < active[index - 1].end())
					return true;

			return false;
		}

		Project project;
		EditHistory history{project};
		Selection selection;
		ClipEditor clips{project, history, selection};
		Clipboard clipboard;
		TrackId track;
	};
}

TEST_CASE("a take over empty space stays on the first lane")
{
	Fixture fixture;
	const auto id = fixture.record(0, 1000, "a.wav");

	CHECK(fixture.project.find_clip(id)->lane == 0);
	CHECK(fixture.project.find_clip(id)->active);
}

TEST_CASE("a take over an earlier one goes to a new lane and silences the overlap")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(400, 1000, "b.wav");

	const auto active = fixture.active_clips();
	REQUIRE(active.size() == 2);
	CHECK(active[0].file == "a.wav");
	CHECK(active[0].end() == 400);
	CHECK(active[1].file == "b.wav");
	CHECK(active[1].lane == 1);
	CHECK(lane_count(*fixture.project.find_track(fixture.track)) == 2);
}

TEST_CASE("comping a range plays it from the chosen lane only")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.comp(0, 200, 600);

	const auto active = fixture.active_clips();
	REQUIRE(active.size() == 3);
	CHECK(active[0].file == "b.wav");
	CHECK(active[1].file == "a.wav");
	CHECK(active[1].start == 200);
	CHECK(active[1].source_offset == 200);
	CHECK(active[1].length == 400);
	CHECK(active[2].file == "b.wav");
}

TEST_CASE("comp edges get short fades so the cut doesn't click")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.comp(0, 200, 600);

	const auto active = fixture.active_clips();
	CHECK(active[0].fade_out == EDGE_FADE);
	CHECK(active[1].fade_in == EDGE_FADE);
	CHECK(active[1].fade_out == EDGE_FADE);
	CHECK(active[2].fade_in == EDGE_FADE);
}

TEST_CASE("comping back to the same take joins it without a fade")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.comp(0, 200, 600);
	fixture.comp(1, 200, 600);

	const auto active = fixture.active_clips();
	REQUIRE(active.size() == 3);

	for (const auto& clip : active)
		CHECK(clip.file == "b.wav");

	CHECK(active[0].fade_out == 0);
	CHECK(active[1].fade_in == 0);
	CHECK(active[1].fade_out == 0);
	CHECK(active[2].fade_in == 0);
}

TEST_CASE("comping where the lane has nothing leaves the other takes playing")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 500, "b.wav");
	fixture.comp(1, 0, 1000);

	const auto active = fixture.active_clips();
	REQUIRE(active.size() == 2);
	CHECK(active[0].file == "b.wav");
	CHECK(active[1].file == "a.wav");
	CHECK(active[1].start == 500);
}

TEST_CASE("undoing a comp restores the takes as they were")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.comp(0, 200, 600);
	fixture.history.undo();

	const auto active = fixture.active_clips();
	REQUIRE(active.size() == 1);
	CHECK(active[0].file == "b.wav");
	CHECK(fixture.project.find_track(fixture.track)->clips.size() == 2);
}

TEST_CASE("a single take doesn't count as takes")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	auto& track = *fixture.project.find_track(fixture.track);
	track.expanded = true;

	CHECK_FALSE(has_takes(track));
	CHECK_FALSE(shows_lanes(track));
}

TEST_CASE("keeping the comp removes the other takes and puts what plays on the first lane")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.comp(0, 200, 600);

	{
		auto edit = fixture.history.begin("Keep Comp");
		keep_comp(edit, fixture.project, fixture.track);
	}

	const auto& track = *fixture.project.find_track(fixture.track);
	CHECK(track.clips.size() == 3);
	CHECK_FALSE(has_takes(track));

	for (const auto& clip : track.clips)
		CHECK(clip.active);
}

TEST_CASE("copying a comp piece pastes every take under it with the same choice")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.select_active_at(0);
	fixture.clipboard.copy(fixture.selection, fixture.project);
	fixture.clips.paste_clips(fixture.clipboard, 2000, SAMPLE_RATE);

	std::vector<Clip> pasted;

	for (const auto& clip : fixture.project.find_track(fixture.track)->clips)
		if (clip.start >= 2000)
			pasted.push_back(clip);

	REQUIRE(pasted.size() == 2);
	CHECK(std::ranges::count_if(pasted, [](const Clip& clip) { return clip.active && clip.file == "b.wav"; }) == 1);
	CHECK(std::ranges::count_if(pasted, [](const Clip& clip) { return !clip.active && clip.file == "a.wav"; }) == 1);
}

TEST_CASE("pasting a comp over takes never plays two takes at once")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.select_active_at(0);
	fixture.clipboard.copy(fixture.selection, fixture.project);
	fixture.clips.paste_clips(fixture.clipboard, 500, SAMPLE_RATE);

	CHECK_FALSE(fixture.active_clips_overlap());
	CHECK(lane_count(*fixture.project.find_track(fixture.track)) == 4);
}

TEST_CASE("deleting a comp piece removes that range from every take")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.comp(0, 200, 600);
	fixture.select_active_at(300);
	fixture.clips.remove_selected();

	for (const auto& clip : fixture.project.find_track(fixture.track)->clips)
		CHECK((clip.end() <= 200 || clip.start >= 600));

	CHECK(lane_count(*fixture.project.find_track(fixture.track)) == 2);
}

TEST_CASE("deleting everything a lane had removes the empty lane")
{
	Fixture fixture;
	fixture.record(0, 1000, "a.wav");
	fixture.record(0, 1000, "b.wav");
	fixture.record(0, 400, "c.wav");
	fixture.select_active_at(0);
	fixture.clips.remove_selected();

	const auto& track = *fixture.project.find_track(fixture.track);
	CHECK(lane_count(track) == 2);
	CHECK(fixture.active_clips().size() == 1);
	CHECK(fixture.active_clips()[0].file == "b.wav");
}
