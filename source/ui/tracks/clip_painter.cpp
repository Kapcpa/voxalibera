#include "ui/tracks/clip_painter.h"

#include "ui/theme.h"

namespace
{
	constexpr float CORNER_RADIUS = 3.0f;
	constexpr float FILL_ALPHA = 0.3f;
	constexpr float WAVEFORM_ALPHA = 0.85f;
	constexpr float SEPARATOR_ALPHA = 0.4f;
	constexpr float NAME_HEIGHT = 15.0f;
	constexpr float NAME_PADDING = 4.0f;
	constexpr float NAME_FONT_SIZE = 11.0f;
	constexpr float SELECTED_BORDER = 2.0f;
	constexpr float NORMAL_BORDER = 1.0f;

	float peak_between(const std::vector<float>& levels, int samples_per_peak, std::int64_t from_sample, std::int64_t to_sample)
	{
		const auto first = (size_t) std::max<std::int64_t>(0, from_sample / samples_per_peak);
		const auto last = std::min(levels.size(), (size_t) std::max<std::int64_t>(0, to_sample / samples_per_peak + 1));
		auto level = 0.0f;

		for (auto index = first; index < last; ++index)
			level = std::max(level, levels[index]);

		return level;
	}

	void paint_channel(juce::Graphics& graphics, juce::Rectangle<float> band, const std::vector<float>& levels, int samples_per_peak, const ClipAppearance& clip)
	{
		const auto to_file = clip.peaks->sample_rate > 0.0 && clip.timeline_rate > 0.0 ? clip.peaks->sample_rate / clip.timeline_rate : 1.0;
		const auto file_offset = (double) clip.source_offset * to_file;
		const auto samples_per_pixel = (double) clip.length * to_file / band.getWidth();
		const auto centre = band.getCentreY();
		const auto half_height = band.getHeight() / 2.0f;

		for (int pixel = 0; pixel < (int) band.getWidth(); ++pixel) {
			const auto from = (std::int64_t) (file_offset + pixel * samples_per_pixel);
			const auto to = (std::int64_t) (file_offset + (pixel + 1) * samples_per_pixel);
			const auto extent = std::min(1.0f, peak_between(levels, samples_per_peak, from, to)) * half_height;
			graphics.drawVerticalLine((int) band.getX() + pixel, centre - extent, centre + extent);
		}
	}

	void paint_waveform(juce::Graphics& graphics, juce::Rectangle<float> area, const ClipAppearance& clip)
	{
		if (clip.peaks == nullptr || clip.peaks->channels.empty() || area.getWidth() < 1.0f)
			return;

		const auto& channels = clip.peaks->channels;
		const auto band_height = area.getHeight() / (float) channels.size();

		for (size_t channel = 0; channel < channels.size(); ++channel) {
			const auto band = area.withHeight(band_height).withY(area.getY() + band_height * (float) channel);
			graphics.setColour(clip.colour.withAlpha(WAVEFORM_ALPHA));
			paint_channel(graphics, band, channels[channel], clip.peaks->samples_per_peak, clip);

			if (channel > 0) {
				graphics.setColour(clip.colour.withAlpha(SEPARATOR_ALPHA));
				graphics.drawHorizontalLine((int) band.getY(), band.getX(), band.getRight());
			}
		}
	}
}

void paint_clip(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ClipAppearance& clip)
{
	const auto colour = clip.recording ? theme::RECORD : clip.colour;

	graphics.setColour(colour.withAlpha(FILL_ALPHA));
	graphics.fillRoundedRectangle(bounds, CORNER_RADIUS);

	auto content = bounds;
	const auto name_area = content.removeFromTop(NAME_HEIGHT);
	paint_waveform(graphics, content, {clip.name, colour, clip.peaks, clip.source_offset, clip.length, clip.timeline_rate});

	graphics.setColour(theme::TEXT);
	graphics.setFont(juce::FontOptions(NAME_FONT_SIZE));
	graphics.drawText(clip.name, name_area.withTrimmedLeft(NAME_PADDING), juce::Justification::centredLeft);

	graphics.setColour(clip.selected ? theme::TEXT : colour);
	graphics.drawRoundedRectangle(bounds, CORNER_RADIUS, clip.selected ? SELECTED_BORDER : NORMAL_BORDER);
}
