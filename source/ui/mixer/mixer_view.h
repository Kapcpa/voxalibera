#pragma once

#include "core/app_context.h"
#include "ui/mixer/mixer_strip.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <map>
#include <memory>

class MixerView : public juce::Component
{
public:
	explicit MixerView(AppContext& context);

	void update();
	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	void sync_strips();
	void layout_strips();
	LevelMeter* meter_for(TrackId id);

	AppContext& context;
	MixerStrip master;
	juce::Viewport viewport;
	juce::Component strip_row;
	std::map<TrackId, std::unique_ptr<MixerStrip>> strips;
	int shown_project_revision = -1;
	int shown_selection_revision = -1;
};
