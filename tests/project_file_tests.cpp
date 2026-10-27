#include "core/project/project_file.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
	bool near(double value, double expected)
	{
		return std::abs(value - expected) < 1.0e-6;
	}

	Project example_project()
	{
		Project project;
		project.sample_rate = 48000.0;
		project.tempo = {96.5, {7, 8}};
		project.metronome = false;
		project.master_volume_db = -3.0f;

		Track guitar;
		guitar.id = project.allocate_track_id();
		guitar.name = "Guitar \"DI\"";
		guitar.colour = TRACK_COLOURS[2].argb;
		guitar.channels = TrackChannels::stereo;
		guitar.input = {2};
		guitar.armed = true;
		guitar.solo = true;
		guitar.volume_db = -6.5f;
		guitar.pan = 0.25f;

		Clip clip;
		clip.id = project.allocate_clip_id();
		clip.file = "audio/Guitar-take-1.wav";
		clip.start = 6000000000;
		clip.source_offset = 512;
		clip.length = 96000;
		clip.gain_db = 1.5f;
		clip.fade_in = 4800;
		clip.fade_out = 960;
		guitar.clips.push_back(clip);

		Plugin amp;
		amp.id = project.allocate_plugin_id();
		amp.format = "VST3";
		amp.identifier = "VST3-Amp-1a2b3c4d-5e6f7a8b";
		amp.name = "Amp";
		amp.manufacturer = "Someone";
		amp.version = "1.2.0";
		amp.bypass = true;
		const char state[] = {0, 1, 2, '"', '\\', 127};
		amp.state.append(state, sizeof(state));
		guitar.plugins.push_back(amp);

		Track vocal;
		vocal.id = project.allocate_track_id();
		vocal.name = "Vocal";
		vocal.input = {TrackInput::NONE};
		vocal.mute = true;

		project.tracks = {guitar, vocal};
		return project;
	}
}

TEST_CASE("a project survives writing and reading back")
{
	const auto original = example_project();
	const auto loaded = read_project(write_project(original, "test"));

	REQUIRE(loaded);
	CHECK(loaded->problems.isEmpty());
	CHECK_FALSE(loaded->newer_format);

	const auto& project = loaded->project;
	CHECK(near(project.sample_rate, 48000.0));
	CHECK(project.tempo == original.tempo);
	CHECK_FALSE(project.metronome);
	CHECK(near(project.master_volume_db, -3.0));
	CHECK(project.next_id == original.next_id);
	REQUIRE(project.tracks.size() == 2);

	const auto& guitar = project.tracks[0];
	const auto& source = original.tracks[0];
	CHECK(guitar.id == source.id);
	CHECK(guitar.name == source.name);
	CHECK(guitar.colour == source.colour);
	CHECK(guitar.channels == TrackChannels::stereo);
	CHECK(guitar.input.first_channel == 2);
	CHECK(guitar.armed);
	CHECK(guitar.solo);
	CHECK(near(guitar.volume_db, -6.5));
	CHECK(near(guitar.pan, 0.25));
	REQUIRE(guitar.clips.size() == 1);
	CHECK(guitar.clips[0].file == source.clips[0].file);
	CHECK(guitar.clips[0].start == source.clips[0].start);
	CHECK(guitar.clips[0].source_offset == 512);
	CHECK(guitar.clips[0].length == 96000);
	CHECK(near(guitar.clips[0].gain_db, 1.5));
	CHECK(guitar.clips[0].fade_in == 4800);
	CHECK(guitar.clips[0].fade_out == 960);
	REQUIRE(guitar.plugins.size() == 1);
	CHECK(guitar.plugins[0].id == source.plugins[0].id);
	CHECK(guitar.plugins[0].format == "VST3");
	CHECK(guitar.plugins[0].identifier == source.plugins[0].identifier);
	CHECK(guitar.plugins[0].name == "Amp");
	CHECK(guitar.plugins[0].manufacturer == "Someone");
	CHECK(guitar.plugins[0].version == "1.2.0");
	CHECK(guitar.plugins[0].bypass);
	CHECK(guitar.plugins[0].state == source.plugins[0].state);

	CHECK_FALSE(project.tracks[1].input.is_connected());
	CHECK(project.tracks[1].mute);
}

TEST_CASE("a damaged project keeps what can be read")
{
	const auto text = R"({
		"format": 1,
		"tempo": {"quarter-notes-per-minute": 9000, "time-signature": [0, 4]},
		"next-id": 1,
		"tracks": [
			{"id": 3, "name": "Kept", "clips": [{"id": 4, "file": "audio/a.wav", "length": 10}, {"id": 5, "length": 10}, {"id": 3, "file": "audio/b.wav", "length": 10}]},
			{"id": 3, "name": "Repeated id"},
			{"name": "No id"}
		]
	})";

	const auto loaded = read_project(text);

	REQUIRE(loaded);
	CHECK(loaded->problems.size() == 4);
	CHECK(near(loaded->project.tempo.quarter_notes_per_minute, Project::MAX_TEMPO));
	CHECK(loaded->project.tempo.signature == TimeSignature{});
	REQUIRE(loaded->project.tracks.size() == 1);
	CHECK(loaded->project.tracks[0].clips.size() == 1);
	CHECK(loaded->project.next_id == 5);
}

TEST_CASE("text that is not a project is refused and newer formats are flagged")
{
	CHECK_FALSE(read_project("not json"));
	CHECK_FALSE(read_project("[1, 2]"));

	const auto newer = read_project(R"({"format": 99, "tracks": []})");
	REQUIRE(newer);
	CHECK(newer->newer_format);
}
