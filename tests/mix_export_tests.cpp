#include "core/audio/mix_export.h"
#include "engine/plugins/plugin_chain.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

namespace
{
	constexpr double SAMPLE_RATE = 48000.0;
	constexpr int FILE_LENGTH = 1000;
	constexpr int CLIP_START = 100;
	constexpr int PLUGIN_LATENCY = 37;
	constexpr int WAIT_STEP_MILLISECONDS = 10;
	constexpr int WAIT_STEPS = 500;
	constexpr float FLOAT_TOLERANCE = 1.0e-6f;
	constexpr float TWO_STEPS_16_BIT = 2.0f / 32768.0f;

	bool near(float value, float expected, float tolerance)
	{
		return std::abs(value - expected) <= tolerance;
	}

	juce::File temporary_wav(const juce::String& name)
	{
		return juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile(name, ".wav");
	}

	juce::File write_ramp_file()
	{
		const auto file = temporary_wav("voxalibera-export-ramp");
		std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
		auto writer = juce::WavAudioFormat().createWriterFor(stream, juce::AudioFormatWriterOptions().withSampleRate(SAMPLE_RATE).withNumChannels(1).withBitsPerSample(32).withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));

		juce::AudioBuffer<float> ramp(1, FILE_LENGTH);

		for (int sample = 0; sample < FILE_LENGTH; ++sample)
			ramp.setSample(0, sample, (float) sample / FILE_LENGTH);

		writer->writeFromAudioSampleBuffer(ramp, 0, FILE_LENGTH);
		return file;
	}

	class DelayProcessor : public PluginProcessor
	{
	public:
		void prepare(const PluginProcessContext&) override {}

		void process(juce::AudioBuffer<float>& buffer, int sample_count, const PluginPlayhead&) override
		{
			auto* samples = buffer.getWritePointer(0);

			for (int index = 0; index < sample_count; ++index) {
				std::swap(samples[index], history[next]);
				next = (next + 1) % history.size();
			}

			buffer.copyFrom(1, 0, buffer, 0, 0, sample_count);
		}

		int latency_samples() const override { return PLUGIN_LATENCY; }

	private:
		std::vector<float> history = std::vector<float>(PLUGIN_LATENCY);
		size_t next = 0;
	};

	ExportResult export_ramp(const juce::File& source, const juce::File& target, int bit_depth)
	{
		TrackPlayback track;
		track.channels = 1;
		track.gains = {1.0f, 1.0f};
		track.clips.push_back({AudioFile::open(source), CLIP_START, FILE_LENGTH, 0.0, 1.0f, nullptr});
		track.plugins = std::make_shared<PluginChain>(std::vector<PluginChain::Slot>{{std::make_shared<DelayProcessor>(), false}});

		ExportJob job;
		job.snapshot = std::make_unique<EngineSnapshot>();
		job.snapshot->tracks.push_back(track);
		job.file = target;
		job.sample_rate = SAMPLE_RATE;
		job.bit_depth = bit_depth;
		job.length = CLIP_START + FILE_LENGTH;
		job.pre_roll = PLUGIN_LATENCY;

		MixExport exporter;
		exporter.start(std::move(job));

		for (int step = 0; step < WAIT_STEPS; ++step) {
			if (auto result = exporter.take_result())
				return *result;

			juce::Thread::sleep(WAIT_STEP_MILLISECONDS);
		}

		return {ExportResult::Outcome::failed, target, 0.0f, false, "timed out"};
	}

	juce::AudioBuffer<float> read_wav(const juce::File& file)
	{
		std::unique_ptr<juce::AudioFormatReader> reader(juce::WavAudioFormat().createReaderFor(file.createInputStream().release(), true));
		juce::AudioBuffer<float> buffer((int) reader->numChannels, (int) reader->lengthInSamples);
		reader->read(&buffer, 0, buffer.getNumSamples(), 0, true, true);
		return buffer;
	}
}

TEST_CASE("an export starts at zero with plugin latency compensated and ends with the last clip")
{
	const auto source = write_ramp_file();
	const auto target = temporary_wav("voxalibera-export");
	const auto result = export_ramp(source, target, export_format::FLOAT_32_BIT);
	const auto mix = read_wav(target);

	REQUIRE(result.outcome == ExportResult::Outcome::finished);
	CHECK(mix.getNumChannels() == 2);
	CHECK(mix.getNumSamples() == CLIP_START + FILE_LENGTH);
	CHECK(near(mix.getSample(0, CLIP_START - 1), 0.0f, FLOAT_TOLERANCE));
	CHECK(near(mix.getSample(0, CLIP_START + 1), 1.0f / FILE_LENGTH, FLOAT_TOLERANCE));
	CHECK(near(mix.getSample(1, CLIP_START + 500), 500.0f / FILE_LENGTH, FLOAT_TOLERANCE));
	CHECK(near(result.peak, (FILE_LENGTH - 1.0f) / FILE_LENGTH, FLOAT_TOLERANCE));

	source.deleteFile();
	target.deleteFile();
}

TEST_CASE("a 16-bit export stays within the dither of the mix")
{
	const auto source = write_ramp_file();
	const auto target = temporary_wav("voxalibera-export-16");
	const auto result = export_ramp(source, target, export_format::DITHERED_16_BIT);
	const auto mix = read_wav(target);

	REQUIRE(result.outcome == ExportResult::Outcome::finished);
	CHECK(near(mix.getSample(0, CLIP_START + 500), 500.0f / FILE_LENGTH, TWO_STEPS_16_BIT));
	CHECK(near(mix.getSample(0, CLIP_START + 900), 900.0f / FILE_LENGTH, TWO_STEPS_16_BIT));

	source.deleteFile();
	target.deleteFile();
}
