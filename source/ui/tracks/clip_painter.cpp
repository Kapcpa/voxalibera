#include "ui/tracks/clip_painter.h"

#include "engine/levels.h"
#include "ui/theme.h"

#include <algorithm>
#include <cmath>
#include <span>

namespace
{
	constexpr float CORNER_RADIUS = 3.0f;
	constexpr float FILL_ALPHA = 0.3f;
	constexpr float HEADER_ALPHA = 0.6f;
	constexpr float WAVEFORM_ALPHA = 0.85f;
	constexpr float SEPARATOR_ALPHA = 0.4f;
	constexpr float NAME_PADDING = 4.0f;
	constexpr float NAME_FONT_SIZE = 11.0f;
	constexpr float SELECTED_BORDER = 2.0f;
	constexpr float NORMAL_BORDER = 1.0f;
	constexpr float WAVEFORM_CURVE = 0.5f;
	constexpr int FADE_CURVE_STEPS = 16;
	constexpr float FADE_SHADE_ALPHA = 0.35f;
	constexpr float FADE_LINE_ALPHA = 0.8f;
	constexpr float FADE_HANDLE_SIZE = 6.0f;
	constexpr float MIN_FADE_WIDTH = 0.5f;
	constexpr float SHOWN_GAIN_DB = 0.05f;

	float displayed_level(float level)
	{
		return std::pow(std::min(1.0f, level), WAVEFORM_CURVE);
	}

	float peak_between(const std::vector<float>& levels, int samples_per_peak, std::int64_t from_sample, std::int64_t to_sample)
	{
		const auto first = (size_t) std::max<std::int64_t>(0, from_sample / samples_per_peak);
		const auto last = std::min(levels.size(), (size_t) std::max<std::int64_t>(0, to_sample / samples_per_peak + 1));
		auto level = 0.0f;

		for (auto index = first; index < last; ++index)
			level = std::max(level, levels[index]);

		return level;
	}

	float mixed_peak_between(std::span<const std::vector<float>> channels, int samples_per_peak, std::int64_t from_sample, std::int64_t to_sample)
	{
		auto sum = 0.0f;

		for (const auto& levels : channels)
			sum += peak_between(levels, samples_per_peak, from_sample, to_sample);

		return sum / (float) channels.size();
	}

	void paint_band(juce::Graphics& graphics, juce::Rectangle<float> band, std::span<const std::vector<float>> channels, int samples_per_peak, const ClipAppearance& clip)
	{
		const auto to_file = clip.peaks->sample_rate > 0.0 && clip.timeline_rate > 0.0 ? clip.peaks->sample_rate / clip.timeline_rate : 1.0;
		const auto file_offset = (double) clip.source_offset * to_file;
		const auto samples_per_pixel = (double) clip.length * to_file / band.getWidth();
		const auto centre = band.getCentreY();
		const auto half_height = band.getHeight() / 2.0f;
		const auto gain = fader_gain(clip.gain_db);

		for (int pixel = 0; pixel < (int) band.getWidth(); ++pixel) {
			const auto position = (std::int64_t) (((float) pixel + 0.5f) / band.getWidth() * (float) clip.length);
			const auto fade = clip_fade_gain(position, clip.length, clip.fade_in, clip.fade_out);
			const auto from = (std::int64_t) (file_offset + pixel * samples_per_pixel);
			const auto to = (std::int64_t) (file_offset + (pixel + 1) * samples_per_pixel);
			const auto extent = displayed_level(gain * fade * mixed_peak_between(channels, samples_per_peak, from, to)) * half_height;
			graphics.drawVerticalLine((int) band.getX() + pixel, centre - extent, centre + extent);
		}
	}

	juce::Path fade_curve(juce::Rectangle<float> area, float start_x, float end_x)
	{
		juce::Path curve;
		curve.startNewSubPath(start_x, area.getBottom());

		for (int step = 1; step <= FADE_CURVE_STEPS; ++step) {
			const auto progress = (float) step / FADE_CURVE_STEPS;
			curve.lineTo(start_x + progress * (end_x - start_x), area.getBottom() - fade_gain(progress) * area.getHeight());
		}

		return curve;
	}

	void paint_fade(juce::Graphics& graphics, juce::Rectangle<float> area, float silent_x, float full_x)
	{
		if (std::abs(full_x - silent_x) < MIN_FADE_WIDTH)
			return;

		const auto curve = fade_curve(area, silent_x, full_x);
		auto shade = curve;
		shade.lineTo(silent_x, area.getY());
		shade.closeSubPath();

		graphics.setColour(theme::WINDOW_BACKGROUND.withAlpha(FADE_SHADE_ALPHA));
		graphics.fillPath(shade);
		graphics.setColour(theme::TEXT.withAlpha(FADE_LINE_ALPHA));
		graphics.strokePath(curve, juce::PathStrokeType(1.0f));
	}

	void paint_fade_handle(juce::Graphics& graphics, juce::Rectangle<float> bounds, juce::Point<float> centre)
	{
		const auto half = FADE_HANDLE_SIZE / 2.0f;
		const auto centre_x = std::clamp(centre.x, bounds.getX() + half, bounds.getRight() - half);
		graphics.setColour(theme::TEXT);
		graphics.fillRect(juce::Rectangle<float>(FADE_HANDLE_SIZE, FADE_HANDLE_SIZE).withCentre({centre_x, centre.y}));
	}

	void paint_waveform(juce::Graphics& graphics, juce::Rectangle<float> area, const ClipAppearance& clip)
	{
		if (clip.peaks == nullptr || clip.peaks->channels.empty() || area.getWidth() < 1.0f)
			return;

		const std::span<const std::vector<float>> channels = clip.peaks->channels;
		const auto samples_per_peak = clip.peaks->samples_per_peak;
		const auto band_count = clip.track_channels == 1 ? 1 : 2;
		const auto band_height = area.getHeight() / (float) band_count;

		for (int band_index = 0; band_index < band_count; ++band_index) {
			const auto band = area.withHeight(band_height).withY(area.getY() + band_height * (float) band_index);
			const auto source = band_count == 1 ? channels : channels.subspan(std::min((size_t) band_index, channels.size() - 1), 1);
			graphics.setColour(clip.colour.withAlpha(WAVEFORM_ALPHA));
			paint_band(graphics, band, source, samples_per_peak, clip);

			if (band_index > 0) {
				graphics.setColour(clip.colour.withAlpha(SEPARATOR_ALPHA));
				graphics.drawHorizontalLine((int) band.getY(), band.getX(), band.getRight());
			}
		}
	}
}

FadeHandles fade_handles(juce::Rectangle<float> bounds, std::int64_t length, std::int64_t fade_in, std::int64_t fade_out)
{
	const auto pixels_per_sample = length > 0 ? bounds.getWidth() / (float) length : 0.0f;
	return {bounds.getX() + (float) fade_in * pixels_per_sample, bounds.getRight() - (float) fade_out * pixels_per_sample, bounds.getY() + CLIP_HEADER_HEIGHT};
}

void paint_clip(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ClipAppearance& clip)
{
	const auto colour = clip.recording ? theme::RECORD : clip.colour;

	graphics.setColour(colour.withAlpha(FILL_ALPHA));
	graphics.fillRoundedRectangle(bounds, CORNER_RADIUS);

	auto content = bounds;
	const auto name_area = content.removeFromTop(CLIP_HEADER_HEIGHT);
	juce::Path header;
	header.addRoundedRectangle(name_area.getX(), name_area.getY(), name_area.getWidth(), name_area.getHeight(), CORNER_RADIUS, CORNER_RADIUS, true, true, false, false);
	graphics.setColour(colour.withAlpha(HEADER_ALPHA));
	graphics.fillPath(header);

	auto waveform = clip;
	waveform.colour = colour;
	paint_waveform(graphics, content, waveform);

	if (!clip.recording && clip.length > 0) {
		const auto handles = fade_handles(bounds, clip.length, clip.fade_in, clip.fade_out);
		paint_fade(graphics, content, bounds.getX(), handles.fade_in_x);
		paint_fade(graphics, content, bounds.getRight(), handles.fade_out_x);

		if (clip.selected) {
			paint_fade_handle(graphics, bounds, {handles.fade_in_x, handles.y});
			paint_fade_handle(graphics, bounds, {handles.fade_out_x, handles.y});
		}
	}

	graphics.setColour(theme::TEXT);
	graphics.setFont(juce::FontOptions(NAME_FONT_SIZE));

	graphics.drawText(clip.name, name_area.withTrimmedLeft(NAME_PADDING), juce::Justification::centredLeft);

	if (std::abs(clip.gain_db) >= SHOWN_GAIN_DB)
		graphics.drawText((clip.gain_db > 0.0f ? "+" : "") + juce::String(clip.gain_db, 1) + " dB", name_area.withTrimmedRight(NAME_PADDING), juce::Justification::centredRight);

	graphics.setColour(clip.selected ? theme::TEXT : colour);
	graphics.drawRoundedRectangle(bounds, CORNER_RADIUS, clip.selected ? SELECTED_BORDER : NORMAL_BORDER);
}
