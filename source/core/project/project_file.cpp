#include "core/project/project_file.h"

#include <set>

namespace
{
	using Object = juce::DynamicObject::Ptr;

	constexpr auto MONO = "mono";
	constexpr auto STEREO = "stereo";

	juce::var read(const juce::var& object, const char* key, const juce::var& fallback)
	{
		const auto* dynamic = object.getDynamicObject();
		return dynamic != nullptr && dynamic->hasProperty(key) ? dynamic->getProperty(key) : fallback;
	}

	std::int64_t read_samples(const juce::var& object, const char* key)
	{
		return (std::int64_t) (juce::int64) read(object, key, 0);
	}

	juce::String colour_text(std::uint32_t argb)
	{
		return "#" + juce::String::toHexString((int) (argb & 0xffffff)).paddedLeft('0', 6);
	}

	std::uint32_t colour_from(const juce::String& text)
	{
		return 0xff000000 | (std::uint32_t) text.fromFirstOccurrenceOf("#", false, false).getHexValue32();
	}

	juce::var write_clip(const Clip& clip)
	{
		Object object = new juce::DynamicObject();
		object->setProperty("id", clip.id.value);
		object->setProperty("file", clip.file);
		object->setProperty("start", (juce::int64) clip.start);
		object->setProperty("source-offset", (juce::int64) clip.source_offset);
		object->setProperty("length", (juce::int64) clip.length);
		object->setProperty("gain-db", clip.gain_db);
		object->setProperty("lane", clip.lane);
		return object.get();
	}

	juce::var write_plugin(const Plugin& plugin)
	{
		Object object = new juce::DynamicObject();
		object->setProperty("id", plugin.id.value);
		object->setProperty("name", plugin.name);
		object->setProperty("format", plugin.format);
		object->setProperty("bypass", plugin.bypass);
		return object.get();
	}

	juce::var write_track(const Track& track)
	{
		juce::Array<juce::var> clips;

		for (const auto& clip : track.clips)
			clips.add(write_clip(clip));

		juce::Array<juce::var> plugins;

		for (const auto& plugin : track.plugins)
			plugins.add(write_plugin(plugin));

		Object object = new juce::DynamicObject();
		object->setProperty("id", track.id.value);
		object->setProperty("name", track.name);
		object->setProperty("colour", colour_text(track.colour));
		object->setProperty("channels", track.channels == TrackChannels::stereo ? STEREO : MONO);
		object->setProperty("input", track.input.is_connected() ? juce::var(track.input.first_channel) : juce::var());
		object->setProperty("armed", track.armed);
		object->setProperty("mute", track.mute);
		object->setProperty("solo", track.solo);
		object->setProperty("volume-db", track.volume_db);
		object->setProperty("pan", track.pan);
		object->setProperty("clips", clips);
		object->setProperty("plugins", plugins);
		return object.get();
	}

	Clip read_clip(const juce::var& json)
	{
		Clip clip;
		clip.id = ClipId{(int) read(json, "id", 0)};
		clip.file = read(json, "file", "").toString();
		clip.start = read_samples(json, "start");
		clip.source_offset = read_samples(json, "source-offset");
		clip.length = read_samples(json, "length");
		clip.gain_db = (float) read(json, "gain-db", 0.0);
		clip.lane = (int) read(json, "lane", 0);
		return clip;
	}

	Plugin read_plugin(const juce::var& json)
	{
		Plugin plugin;
		plugin.id = PluginId{(int) read(json, "id", 0)};
		plugin.name = read(json, "name", "").toString();
		plugin.format = read(json, "format", "").toString();
		plugin.bypass = (bool) read(json, "bypass", false);
		return plugin;
	}

	Track read_track(const juce::var& json)
	{
		const auto input = read(json, "input", juce::var());

		Track track;
		track.id = TrackId{(int) read(json, "id", 0)};
		track.name = read(json, "name", "").toString();
		track.colour = colour_from(read(json, "colour", "#808080").toString());
		track.channels = read(json, "channels", MONO).toString() == STEREO ? TrackChannels::stereo : TrackChannels::mono;
		track.input = TrackInput{input.isVoid() ? TrackInput::NONE : (int) input};
		track.armed = (bool) read(json, "armed", false);
		track.mute = (bool) read(json, "mute", false);
		track.solo = (bool) read(json, "solo", false);
		track.volume_db = (float) read(json, "volume-db", 0.0);
		track.pan = (float) read(json, "pan", 0.0);

		if (const auto* clips = read(json, "clips", juce::var()).getArray())
			for (const auto& clip : *clips)
				track.clips.push_back(read_clip(clip));

		if (const auto* plugins = read(json, "plugins", juce::var()).getArray())
			for (const auto& plugin : *plugins)
				track.plugins.push_back(read_plugin(plugin));

		return track;
	}

	bool is_usable(const Clip& clip)
	{
		return clip.id.is_valid() && clip.file.isNotEmpty() && clip.length > 0 && clip.start >= 0 && clip.source_offset >= 0;
	}

	bool is_usable(const Plugin& plugin)
	{
		return plugin.id.is_valid() && plugin.name.isNotEmpty();
	}

	void keep_valid_parts(Project& project, juce::StringArray& problems)
	{
		std::set<int> ids;

		const auto claim = [&](int id) { return id > 0 && ids.insert(id).second; };

		std::erase_if(project.tracks, [&](const Track& track) {
			if (claim(track.id.value))
				return false;

			problems.add("Skipped a track with a missing or repeated id (" + juce::String(track.id.value) + ")");
			return true;
		});

		for (auto& track : project.tracks) {
			std::erase_if(track.clips, [&](const Clip& clip) {
				if (is_usable(clip) && claim(clip.id.value))
					return false;

				problems.add("Skipped a broken clip on track \"" + track.name + "\"");
				return true;
			});

			std::erase_if(track.plugins, [&](const Plugin& plugin) {
				if (is_usable(plugin) && claim(plugin.id.value))
					return false;

				problems.add("Skipped a broken plugin on track \"" + track.name + "\"");
				return true;
			});
		}

		const auto highest = ids.empty() ? 0 : *ids.rbegin();
		project.next_id = std::max(project.next_id, highest + 1);
	}
}

juce::String write_project(const Project& project, const juce::String& app_version)
{
	juce::Array<juce::var> tracks;

	for (const auto& track : project.tracks)
		tracks.add(write_track(track));

	Object tempo = new juce::DynamicObject();
	tempo->setProperty("quarter-notes-per-minute", project.tempo.quarter_notes_per_minute);
	tempo->setProperty("time-signature", juce::Array<juce::var>{project.tempo.signature.beats_per_bar, project.tempo.signature.beat_unit});

	Object root = new juce::DynamicObject();
	root->setProperty("format", PROJECT_FORMAT);
	root->setProperty("app-version", app_version);
	root->setProperty("sample-rate", project.sample_rate);
	root->setProperty("tempo", tempo.get());
	root->setProperty("metronome", project.metronome);
	root->setProperty("count-in", project.count_in);
	root->setProperty("master-volume-db", project.master_volume_db);
	root->setProperty("next-id", project.next_id);
	root->setProperty("tracks", tracks);
	return juce::JSON::toString(root.get());
}

std::optional<LoadedProject> read_project(const juce::String& text)
{
	const auto json = juce::JSON::parse(text);

	if (json.getDynamicObject() == nullptr)
		return std::nullopt;

	LoadedProject loaded;
	auto& project = loaded.project;
	const auto tempo = read(json, "tempo", juce::var());
	const auto signature = read(tempo, "time-signature", juce::var());

	loaded.newer_format = (int) read(json, "format", PROJECT_FORMAT) > PROJECT_FORMAT;
	project.sample_rate = (double) read(json, "sample-rate", 0.0);
	project.tempo.quarter_notes_per_minute = std::clamp((double) read(tempo, "quarter-notes-per-minute", 120.0), Project::MIN_TEMPO, Project::MAX_TEMPO);

	if (const auto* parts = signature.getArray(); parts != nullptr && parts->size() == 2)
		project.tempo.signature = parse_time_signature(juce::String((int) (*parts)[0]) + "/" + juce::String((int) (*parts)[1])).value_or(TimeSignature{});

	project.metronome = (bool) read(json, "metronome", true);
	project.count_in = (bool) read(json, "count-in", false);
	project.master_volume_db = (float) read(json, "master-volume-db", 0.0);
	project.next_id = (int) read(json, "next-id", 1);

	if (const auto* tracks = read(json, "tracks", juce::var()).getArray())
		for (const auto& track : *tracks)
			project.tracks.push_back(read_track(track));

	keep_valid_parts(project, loaded.problems);
	return loaded;
}
