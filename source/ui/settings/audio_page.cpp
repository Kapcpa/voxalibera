#include "ui/settings/audio_page.h"

#include "ui/settings/settings_layout.h"

AudioPage::AudioPage(AppContext& context) : device_picker(context), monitoring_section(context), status_section(context)
{
	addAndMakeVisible(device_label);
	addAndMakeVisible(device_picker);
	addAndMakeVisible(monitoring_label);
	addAndMakeVisible(monitoring_section);
	addAndMakeVisible(status_label);
	addAndMakeVisible(status_section);
}

void AudioPage::update()
{
	device_picker.update();
	monitoring_section.update();
	status_section.update();
}

void AudioPage::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds();

	device_label.setBounds(area.removeFromTop(SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);
	device_picker.setBounds(area.removeFromTop(DevicePicker::preferred_height()));
	area.removeFromTop(SECTION_GAP - ROW_GAP);

	monitoring_label.setBounds(area.removeFromTop(SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);
	monitoring_section.setBounds(area.removeFromTop(MonitoringSection::preferred_height()));
	area.removeFromTop(SECTION_GAP);

	status_label.setBounds(area.removeFromTop(SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);
	status_section.setBounds(area.removeFromTop(StatusSection::preferred_height()));
}
