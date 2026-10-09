#pragma once

#include "core/app_context.h"
#include "ui/settings/device_picker.h"
#include "ui/settings/monitoring_section.h"
#include "ui/settings/section_label.h"
#include "ui/settings/settings_page.h"
#include "ui/settings/status_section.h"

class AudioPage : public SettingsPage
{
public:
	explicit AudioPage(AppContext& context);

	void update() override;
	void resized() override;

private:
	SectionLabel device_label{"Device"};
	DevicePicker device_picker;
	SectionLabel monitoring_label{"Monitoring"};
	MonitoringSection monitoring_section;
	SectionLabel status_label{"Status"};
	StatusSection status_section;
};
