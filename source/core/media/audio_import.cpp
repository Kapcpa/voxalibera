#include "core/media/audio_import.h"

#include "core/app_context.h"
#include "core/media/project_folders.h"

namespace
{
	constexpr int BITS_PER_SAMPLE = 24;
	constexpr auto WAV_EXTENSION = ".wav";

	std::unique_ptr<juce::AudioFormatReader> open_reader(const juce::File& file)
	{
		juce::AudioFormatManager formats;
		formats.registerBasicFormats();
		return std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(file));
	}

	bool convert_to_wav(juce::AudioFormatReader& reader, const juce::File& target)
	{
		std::unique_ptr<juce::OutputStream> stream = target.createOutputStream();

		if (stream == nullptr)
			return false;

		const auto options = juce::AudioFormatWriterOptions().withSampleRate(reader.sampleRate).withNumChannels((int) reader.numChannels).withBitsPerSample(BITS_PER_SAMPLE);
		auto writer = juce::WavAudioFormat().createWriterFor(stream, options);
		return writer != nullptr && writer->writeFromAudioReader(reader, 0, -1);
	}

	bool place_in_project(const juce::File& source, juce::AudioFormatReader& reader, const juce::File& target)
	{
		if (source.hasFileExtension(WAV_EXTENSION))
			return source.copyFileTo(target);

		return convert_to_wav(reader, target);
	}
}

juce::String importable_audio_wildcard()
{
	juce::AudioFormatManager formats;
	formats.registerBasicFormats();
	return formats.getWildcardForAllFormats();
}

bool import_audio_file(AppContext& context, const juce::File& file)
{
	auto source_reader = open_reader(file);

	if (source_reader == nullptr)
		return false;

	auto& project = context.project;

	if (project.folder == juce::File())
		project.folder = new_unsaved_project_folder();

	const auto target = project.folder.getChildFile(AUDIO_FOLDER).getNonexistentChildFile(file.getFileNameWithoutExtension(), WAV_EXTENSION);

	if (!place_in_project(file, *source_reader, target)) {
		target.deleteFile();
		return false;
	}

	const auto reader = open_reader(target);

	if (reader == nullptr) {
		target.deleteFile();
		return false;
	}

	const auto timeline = timeline_rate(project, context.engine.sample_rate.load());

	if (project.sample_rate <= 0.0)
		project.sample_rate = timeline;

	Clip clip;
	clip.file = relative_media_path(project.folder, target);
	clip.length = convert_samples(reader->lengthInSamples, reader->sampleRate, timeline);

	const auto channels = reader->numChannels == 1 ? TrackChannels::mono : TrackChannels::stereo;
	auto edit = context.edits.begin("Import");
	const auto track = context.tracks.add(edit, {file.getFileNameWithoutExtension(), channels, TrackInput{TrackInput::NONE}});
	edit.add_clip(track, clip);

	context.peak_builder.request(project.folder, clip.file);
	return true;
}
