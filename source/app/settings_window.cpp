#include "app/settings_window.h"

#include "ui/theme.h"

SettingsWindow::SettingsWindow(AppContext& context) : DocumentWindow("Audio Settings", theme::WINDOW_BACKGROUND, closeButton), audio_settings_view(context)
{
	setUsingNativeTitleBar(true);
	setContentNonOwned(&audio_settings_view, true);
	setResizable(true, false);
	centreWithSize(getWidth(), getHeight());
}

void SettingsWindow::update()
{
	if (isVisible())
		audio_settings_view.update();
}

void SettingsWindow::closeButtonPressed()
{
	setVisible(false);
}
