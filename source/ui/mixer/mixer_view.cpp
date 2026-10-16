#include "ui/mixer/mixer_view.h"

#include "ui/theme.h"

MixerView::MixerView(AppContext& context) : context(context), master(context, TrackId{})
{
	viewport.setViewedComponent(&strip_row, false);
	viewport.setScrollBarsShown(false, true);
	addAndMakeVisible(master);
	addAndMakeVisible(viewport);
}

LevelMeter* MixerView::meter_for(TrackId id)
{
	const auto meter = context.publisher.meter_of(id);
	return meter == NO_METER ? nullptr : &context.engine.mixer.track_meters[(size_t) meter];
}

void MixerView::update()
{
	if (context.project.revision != shown_project_revision || context.selection.revision != shown_selection_revision) {
		shown_project_revision = context.project.revision;
		shown_selection_revision = context.selection.revision;
		sync_strips();
		layout_strips();
	}

	master.update_meter();

	for (auto& [id, strip] : strips)
		strip->update_meter();
}

void MixerView::sync_strips()
{
	std::erase_if(strips, [&](const auto& entry) { return context.project.find_track(entry.first) == nullptr; });

	for (const auto& track : context.project.tracks) {
		auto& strip = strips[track.id];

		if (strip == nullptr) {
			strip = std::make_unique<MixerStrip>(context, track.id);
			strip->on_show_plugins = [this](TrackId id) {
				if (on_show_plugins)
					on_show_plugins(id);
			};
			strip_row.addAndMakeVisible(*strip);
		}

		strip->show_track(track, context.selection.contains(track.id), meter_for(track.id));
	}

	master.show_master(context.project.master_volume_db, &context.engine.mixer.master_meter);
}

void MixerView::layout_strips()
{
	const auto& tracks = context.project.tracks;
	strip_row.setSize((int) tracks.size() * MixerStrip::WIDTH, viewport.getMaximumVisibleHeight());

	for (size_t position = 0; position < tracks.size(); ++position)
		strips[tracks[position].id]->setBounds((int) position * MixerStrip::WIDTH, 0, MixerStrip::WIDTH, strip_row.getHeight());
}

void MixerView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(0, 0.0f, (float) getWidth());
}

void MixerView::resized()
{
	auto area = getLocalBounds().withTrimmedTop(1);
	master.setBounds(area.removeFromLeft(MixerStrip::WIDTH));
	viewport.setBounds(area);
	layout_strips();
}
