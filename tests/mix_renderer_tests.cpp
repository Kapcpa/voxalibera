#include "engine/mix_renderer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
	constexpr double SAMPLE_RATE = 48000.0;
	constexpr int FILE_LENGTH = 1000;

	juce::File write_ramp_file()
	{
		const auto file = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("voxalibera-ramp", ".wav");
		std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
		auto writer = juce::WavAudioFormat().createWriterFor(stream, juce::AudioFormatWriterOptions().withSampleRate(SAMPLE_RATE).withNumChannels(1).withBitsPerSample(32).withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));

		juce::AudioBuffer<float> ramp(1, FILE_LENGTH);

		for (int sample = 0; sample < FILE_LENGTH; ++sample)
			ramp.setSample(0, sample, (float) sample / FILE_LENGTH);

		writer->writeFromAudioSampleBuffer(ramp, 0, FILE_LENGTH);
		return file;
	}

	bool near(float value, float expected)
	{
		return std::abs(value - expected) < 0.0001f;
	}
}

TEST_CASE("a clip plays the right part of its file at the right place and level")
{
	const auto file = write_ramp_file();

	TrackPlayback track;
	track.channels = 1;
	track.gains = {0.5f, 1.0f};
	track.clips.push_back({AudioFile::open(file), 100, 500, 200.0, 1.0f});

	EngineSnapshot snapshot;
	snapshot.tracks.push_back(track);

	std::vector<float> left(256), right(256);
	float* outputs[]{left.data(), right.data()};
	MixRenderer renderer;
	renderer.render(snapshot, {}, outputs, 2, 256, {true, false, 0});

	CHECK(near(left[99], 0.0f));
	CHECK(near(left[100], 0.5f * 200.0f / FILE_LENGTH));
	CHECK(near(right[100], 200.0f / FILE_LENGTH));
	CHECK(near(right[255], 355.0f / FILE_LENGTH));

	file.deleteFile();
}

TEST_CASE("nothing plays from clips while the transport is stopped")
{
	const auto file = write_ramp_file();

	TrackPlayback track;
	track.clips.push_back({AudioFile::open(file), 0, FILE_LENGTH, 0.0, 1.0f});

	EngineSnapshot snapshot;
	snapshot.tracks.push_back(track);

	std::vector<float> left(64, 1.0f), right(64, 1.0f);
	float* outputs[]{left.data(), right.data()};
	MixRenderer().render(snapshot, {}, outputs, 2, 64, {false, false, 500});

	CHECK(near(left[63], 0.0f));
	CHECK(near(right[63], 0.0f));
	file.deleteFile();
}

TEST_CASE("a clip at another sample rate is resampled to the device")
{
	const auto file = write_ramp_file();
	const auto step = SAMPLE_RATE / 44100.0;

	TrackPlayback track;
	track.channels = 1;
	track.gains = {1.0f, 1.0f};
	track.clips.push_back({AudioFile::open(file), 0, 900, 0.0, 1.0f, std::make_shared<const ResamplingKernel>(step)});

	EngineSnapshot snapshot;
	snapshot.tracks.push_back(track);

	std::vector<float> left(1024), right(1024);
	float* outputs[]{left.data(), right.data()};
	MixRenderer().render(snapshot, {}, outputs, 2, 1024, {true, false, 0});

	CHECK(std::abs(right[400] - (float) (400.0 * step / FILE_LENGTH)) < 0.005f);
	CHECK(std::abs(right[800] - (float) (800.0 * step / FILE_LENGTH)) < 0.005f);
	CHECK(near(right[950], 0.0f));
	file.deleteFile();
}
