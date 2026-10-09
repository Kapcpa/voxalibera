#include "ui/audio_settings_view.h"

namespace
{
constexpr int WIDTH = 1140;
constexpr int HEIGHT = 820;
constexpr int MARGIN = 12;
constexpr int SECTION_GAP = 12;
constexpr int COLUMN_GAP = 16;
constexpr int HARDWARE_COLUMN_WIDTH = 380;
constexpr int DEVICE_SELECTOR_HEIGHT = 420;
constexpr int MONITOR_HEIGHT = 64;
constexpr int LEVEL_METER_HEIGHT = 70;

constexpr int MIN_CHANNELS = 1;
constexpr int MAX_CHANNELS = 2;
constexpr bool SHOW_MIDI_INPUTS = false;
constexpr bool SHOW_MIDI_OUTPUTS = false;
constexpr bool CHANNELS_AS_STEREO_PAIRS = false;
constexpr bool HIDE_ADVANCED_OPTIONS = false;
}

AudioSettingsView::AudioSettingsView(AppContext& context)
	: device_selector(context.engine.device_manager, MIN_CHANNELS, MAX_CHANNELS, MIN_CHANNELS, MAX_CHANNELS, SHOW_MIDI_INPUTS, SHOW_MIDI_OUTPUTS, CHANNELS_AS_STEREO_PAIRS, HIDE_ADVANCED_OPTIONS),
	  monitor_view(context),
	  level_meter_view(context.engine.input_meter),
	  diagnostics_view(context),
	  hardware_controls_view(context)
{
	addAndMakeVisible(device_selector);
	addAndMakeVisible(monitor_view);
	addAndMakeVisible(level_meter_view);
	addAndMakeVisible(diagnostics_view);
	addAndMakeVisible(hardware_controls_view);

	setSize(WIDTH, HEIGHT);
}

void AudioSettingsView::update()
{
	monitor_view.update();
	level_meter_view.update();
	diagnostics_view.update();
	hardware_controls_view.update();
}

void AudioSettingsView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void AudioSettingsView::resized()
{
	auto area = getLocalBounds().reduced(MARGIN);

	hardware_controls_view.setBounds(area.removeFromRight(HARDWARE_COLUMN_WIDTH));
	area.removeFromRight(COLUMN_GAP);

	device_selector.setBounds(area.removeFromTop(DEVICE_SELECTOR_HEIGHT));
	area.removeFromTop(SECTION_GAP);
	monitor_view.setBounds(area.removeFromTop(MONITOR_HEIGHT));
	area.removeFromTop(SECTION_GAP);
	level_meter_view.setBounds(area.removeFromTop(LEVEL_METER_HEIGHT));
	area.removeFromTop(SECTION_GAP);
	diagnostics_view.setBounds(area);
}
