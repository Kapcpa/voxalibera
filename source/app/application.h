#pragma once

#include "app/main_window.h"
#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class Application : public juce::JUCEApplication
{
public:
	const juce::String getApplicationName() override;
	const juce::String getApplicationVersion() override;
	bool moreThanOneInstanceAllowed() override;

	void initialise(const juce::String& command_line) override;
	void shutdown() override;
	void systemRequestedQuit() override;

private:
	std::unique_ptr<AppContext> context;
	std::unique_ptr<MainWindow> window;
};
