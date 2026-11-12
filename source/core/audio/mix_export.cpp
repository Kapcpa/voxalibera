#include "core/audio/mix_export.h"

#include "engine/mix_renderer.h"

namespace
{
	constexpr int CHANNELS = 2;
	constexpr int STOP_TIMEOUT_MILLISECONDS = 5000;
	constexpr float SILENCE = 1.0e-5f;
	constexpr float LEAST_SIGNIFICANT_BIT_16 = 1.0f / 32768.0f;

	juce::AudioFormatWriterOptions writer_options(const ExportJob& job)
	{
		const auto options = juce::AudioFormatWriterOptions().withSampleRate(job.sample_rate).withNumChannels(CHANNELS).withBitsPerSample(job.bit_depth);

		if (job.bit_depth == export_format::FLOAT_32_BIT)
			return options.withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);

		return options;
	}
}

MixExport::MixExport() : juce::Thread("Mix export")
{
}

MixExport::~MixExport()
{
	stopThread(STOP_TIMEOUT_MILLISECONDS);
}

void MixExport::start(ExportJob new_job)
{
	stopThread(STOP_TIMEOUT_MILLISECONDS);
	job = std::move(new_job);
	result = {};
	rendered = 0.0;
	done = false;
	startThread();
}

void MixExport::cancel()
{
	signalThreadShouldExit();
}

std::optional<ExportResult> MixExport::take_result()
{
	if (!done.load(std::memory_order_acquire))
		return std::nullopt;

	stopThread(STOP_TIMEOUT_MILLISECONDS);
	done = false;
	job = {};
	return result;
}

void MixExport::run()
{
	result = render();
	done.store(true, std::memory_order_release);
}

ExportResult MixExport::render()
{
	juce::TemporaryFile temporary(job.file);
	std::unique_ptr<juce::OutputStream> stream = temporary.getFile().createOutputStream();

	if (stream == nullptr)
		return failure("Couldn't write to " + job.file.getParentDirectory().getFullPathName() + ".");

	auto writer = juce::WavAudioFormat().createWriterFor(stream, writer_options(job));

	if (writer == nullptr)
		return failure("Couldn't create a WAV file with these settings.");

	const auto renderer = std::make_unique<MixRenderer>();
	juce::AudioBuffer<float> mix(CHANNELS, MixRenderer::CHUNK_SAMPLES);
	const auto end = job.length + job.tail;
	const auto total = (double) (end + job.pre_roll);
	auto peak = 0.0f;

	for (auto position = -job.pre_roll; position < end; position += MixRenderer::CHUNK_SAMPLES) {
		if (threadShouldExit())
			return {ExportResult::Outcome::cancelled, job.file, 0.0f, false, {}};

		const auto count = (int) std::min<std::int64_t>(MixRenderer::CHUNK_SAMPLES, end - position);
		renderer->render(*job.snapshot, {}, mix.getArrayOfWritePointers(), CHANNELS, count, {true, false, position});

		const auto skip = (int) std::clamp<std::int64_t>(-position, 0, count);
		const auto audible = count - skip;

		if (audible == 0)
			continue;

		const auto block_peak = mix.getMagnitude(skip, audible);

		if (position >= job.length && block_peak < SILENCE)
			break;

		peak = std::max(peak, block_peak);

		if (job.bit_depth == export_format::DITHERED_16_BIT)
			add_dither(mix, skip, audible);

		if (!writer->writeFromAudioSampleBuffer(mix, skip, audible))
			return failure("Couldn't write the file. Is the disk full?");

		rendered.store((double) (position + count + job.pre_roll) / total, std::memory_order_relaxed);
	}

	writer.reset();

	if (!temporary.overwriteTargetFileWithTemporary())
		return failure("Couldn't replace " + job.file.getFullPathName() + ".");

	const auto clipped = peak > 1.0f && job.bit_depth != export_format::FLOAT_32_BIT;
	return {ExportResult::Outcome::finished, job.file, peak, clipped, {}};
}

ExportResult MixExport::failure(const juce::String& problem) const
{
	return {ExportResult::Outcome::failed, job.file, 0.0f, false, problem};
}

void MixExport::add_dither(juce::AudioBuffer<float>& buffer, int start, int count)
{
	for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
		auto* samples = buffer.getWritePointer(channel, start);

		for (int sample = 0; sample < count; ++sample)
			samples[sample] += (random.nextFloat() - random.nextFloat()) * LEAST_SIGNIFICANT_BIT_16;
	}
}
