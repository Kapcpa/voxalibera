#include "app/main_window.h"

#include "ui/main_view.h"

MainWindow::MainWindow(const juce::String& name, AppContext& context)
	: DocumentWindow(name, juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), allButtons)
{
	setUsingNativeTitleBar(true);
	setContentOwned(new MainView(context), true);
	setResizable(true, true);
	centreWithSize(getWidth(), getHeight());
	setVisible(true);
}

void MainWindow::closeButtonPressed()
{
	juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
