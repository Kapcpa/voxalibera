#include "core/media/peaks.h"

#include "core/log.h"
#include "core/media/project_folders.h"
#include "engine/audio_file.h"
#include "engine/recording_session.h"

#include <cmath>

namespace
{
	constexpr auto CACHE_EXTENSION = ".peaks";
	constexpr int CACHE_MAGIC = 0x3250584b;
	constexpr int PEAKS_PER_READ = 256;
	constexpr int MAX_CHANNELS = RecordingTake::MAX_CHANNELS;

	std::optional<Peaks> read_cache(const juce::File& cache, const juce::File& audio)
	{
		if (!cache.existsAsFile() || cache.getLastModificationTime() < audio.getLastModificationTime())
			return std::nullopt;

		juce::FileInputStream stream(cache);

		if (!stream.openedOk() || stream.readInt() != CACHE_MAGIC)
			return std::nullopt;

		Peaks peaks;
		peaks.samples_per_peak = stream.readInt();
		peaks.sample_rate = stream.readDouble();
		const auto channels = stream.readInt();
		const auto count = stream.readInt();

		if (peaks.samples_per_peak <= 0 || peaks.sample_rate <= 0.0 || channels < 1 || channels > MAX_CHANNELS || count < 0 || stream.getNumBytesRemaining() != (juce::int64) channels * count * (juce::int64) sizeof(float))
			return std::nullopt;

		for (int channel = 0; channel < channels; ++channel) {
			auto& values = peaks.channels.emplace_back((size_t) count);
			stream.read(values.data(), count * (int) sizeof(float));
		}

		return peaks;
	}

	void write_cache(const juce::File& cache, const Peaks& peaks)
	{
		cache.getParentDirectory().createDirectory();
		juce::FileOutputStream stream(cache);

		if (!stream.openedOk())
			return;

		stream.setPosition(0);
		stream.truncate();
		stream.writeInt(CACHE_MAGIC);
		stream.writeInt(peaks.samples_per_peak);
		stream.writeDouble(peaks.sample_rate);
		stream.writeInt((int) peaks.channels.size());
		stream.writeInt((int) peaks.channels.front().size());

		for (const auto& values : peaks.channels)
			stream.write(values.data(), values.size() * sizeof(float));
	}
}

const Peaks* PeakStore::find(const juce::String& file) const
{
	const auto found = peaks_by_file.find(file);
	return found == peaks_by_file.end() ? nullptr : &found->second;
}

void PeakStore::store(const juce::String& file, Peaks peaks)
{
	peaks_by_file[file] = std::move(peaks);
	++revision;
}

void PeakStore::clear()
{
	peaks_by_file.clear();
	++revision;
}

juce::File peak_cache_file(const juce::File& project_folder, const juce::String& file)
{
	return project_folder.getChildFile(CACHE_FOLDER).getChildFile(file + CACHE_EXTENSION);
}

PeakBuilder::PeakBuilder() : juce::Thread("Peak builder")
{
	startThread(juce::Thread::Priority::background);
}

PeakBuilder::~PeakBuilder()
{
	signalThreadShouldExit();
	notify();
	stopThread(-1);
}

void PeakBuilder::request(const juce::File& project_folder, const juce::String& file)
{
	{
		const std::scoped_lock lock(mutex);
		jobs.push_back({project_folder.getChildFile(file), peak_cache_file(project_folder, file), file, generation});
	}

	notify();
}

void PeakBuilder::cancel_all()
{
	const std::scoped_lock lock(mutex);
	jobs.clear();
	finished.clear();
	++generation;
}

void PeakBuilder::deliver_to(PeakStore& store)
{
	const std::scoped_lock lock(mutex);

	for (auto& [file, peaks] : finished)
		store.store(file, std::move(peaks));

	finished.clear();
}

std::optional<PeakBuilder::Job> PeakBuilder::next_job()
{
	const std::scoped_lock lock(mutex);

	if (jobs.empty())
		return std::nullopt;

	auto job = jobs.front();
	jobs.pop_front();
	return job;
}

void PeakBuilder::run()
{
	while (!threadShouldExit()) {
		const auto job = next_job();

		if (!job) {
			wait(-1);
			continue;
		}

		auto peaks = build(*job);

		const std::scoped_lock lock(mutex);

		if (peaks && job->generation == generation)
			finished.emplace_back(job->file, std::move(*peaks));
	}
}

std::optional<Peaks> PeakBuilder::build(const Job& job)
{
	if (auto cached = read_cache(job.cache, job.audio))
		return cached;

	const auto file = AudioFile::open(job.audio);

	if (file == nullptr) {
		log_event("Could not read " + job.audio.getFullPathName() + " for its waveform");
		return std::nullopt;
	}

	const auto channels = std::min(file->channels(), MAX_CHANNELS);
	const auto block = RecordingTake::SAMPLES_PER_PEAK * PEAKS_PER_READ;
	juce::AudioBuffer<float> buffer(file->channels(), block);
	const auto pointers = buffer.getArrayOfWritePointers();

	Peaks peaks{RecordingTake::SAMPLES_PER_PEAK, std::vector<std::vector<float>>((size_t) channels), file->sample_rate()};

	for (std::int64_t start = 0; start < file->length(); start += block) {
		if (threadShouldExit())
			return std::nullopt;

		const auto count = (int) std::min<std::int64_t>(block, file->length() - start);
		file->read(pointers, start, count);

		for (int channel = 0; channel < channels; ++channel)
			for (int first = 0; first < count; first += RecordingTake::SAMPLES_PER_PEAK) {
				const auto* samples = pointers[channel] + first;
				const auto length = std::min(RecordingTake::SAMPLES_PER_PEAK, count - first);
				float peak = 0.0f;

				for (int sample = 0; sample < length; ++sample)
					peak = std::max(peak, std::abs(samples[sample]));

				peaks.channels[(size_t) channel].push_back(peak);
			}
	}

	if (!peaks.channels.empty())
		write_cache(job.cache, peaks);

	return peaks;
}
