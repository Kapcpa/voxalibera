#pragma once

#include <juce_core/juce_core.h>

#include <map>
#include <vector>

struct Peaks
{
	int samples_per_peak = 0;
	std::vector<std::vector<float>> channels;
};

class PeakStore
{
public:
	const Peaks* find(const juce::String& file) const;
	void store(const juce::String& file, Peaks peaks);

private:
	std::map<juce::String, Peaks> peaks_by_file;
};
