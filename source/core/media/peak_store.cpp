#include "core/media/peak_store.h"

const Peaks* PeakStore::find(const juce::String& file) const
{
	const auto found = peaks_by_file.find(file);
	return found == peaks_by_file.end() ? nullptr : &found->second;
}

void PeakStore::store(const juce::String& file, Peaks peaks)
{
	peaks_by_file[file] = std::move(peaks);
}

void PeakStore::clear()
{
	peaks_by_file.clear();
}
