#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

class DiagnosticsView : public juce::Component
{
public:
	explicit DiagnosticsView(AppContext& context);

	void update();
	void resized() override;

private:
	juce::StringArray describe_device(juce::AudioIODevice& device) const;

	AppContext& context;
	juce::Label text;
	int frames_until_refresh = 0;
};
