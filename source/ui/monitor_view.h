#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MonitorView : public juce::Component
{
public:
	explicit MonitorView(AppContext& context);

	void update();
	void resized() override;

private:
	void set_monitoring(bool monitoring);
	void set_routing(InputRouting routing);
	void set_jack_buffer_size(int buffer_size);

	AppContext& context;
	juce::ToggleButton monitoring_toggle{"Monitor (input -> output)"};
	juce::Label routing_label{{}, "Source:"};
	juce::ComboBox routing_box;
	juce::TextButton rescan_button{"Rescan devices"};
	juce::Label jack_buffer_label{{}, "JACK buffer (PipeWire quantum):"};
	juce::ComboBox jack_buffer_box;
};
