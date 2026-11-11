#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

class ExportDialog : public juce::Component
{
public:
	explicit ExportDialog(AppContext& context);

	void prepare();
	void update();
	void resized() override;

	std::function<void()> on_closed;
	std::function<void(const ExportResult&)> on_finished;

private:
	void choose_file();
	void use_file(const juce::File& chosen);
	void start();
	void cancel();
	void show_running(bool running);
	juce::File default_file() const;
	juce::String describe_mix() const;

	AppContext& context;
	juce::File file;
	juce::File file_project;
	juce::Label file_title{{}, "File"};
	juce::Label file_label;
	juce::TextButton choose_button{"Choose..."};
	juce::Label format_title{{}, "Format"};
	juce::ComboBox format;
	juce::Label details;
	double progress = 0.0;
	juce::ProgressBar progress_bar{progress};
	juce::TextButton cancel_button{"Cancel"};
	juce::TextButton export_button{"Export"};
	std::unique_ptr<juce::FileChooser> chooser;
};
