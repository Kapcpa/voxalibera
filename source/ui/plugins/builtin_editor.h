#pragma once

#include "engine/plugins/builtin/builtin_processor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

class BuiltinEditor : public juce::Component
{
public:
	using FileImport = std::function<juce::String(const juce::File&)>;

	BuiltinEditor(BuiltinProcessor& processor, const juce::String& title, FileImport import_file, juce::String file_wildcard);

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	struct Control
	{
		juce::Label name;
		std::unique_ptr<juce::Slider> slider;
		std::unique_ptr<juce::ComboBox> choice;
	};

	struct FileRow
	{
		BuiltinFile& file;
		juce::Label name;
		juce::Label current;
		juce::TextButton load{"Load..."};
		juce::TextButton clear{"Clear"};
	};

	void add_control(BuiltinParameter& parameter);
	void add_slider(Control& control, BuiltinParameter& parameter);
	void add_choice(Control& control, BuiltinParameter& parameter);
	void add_file_row(BuiltinFile& file);
	void choose_file(FileRow& row);
	void show_file(FileRow& row);
	int column_count() const;
	int files_height() const;

	BuiltinProcessor& processor;
	juce::String title;
	FileImport import_file;
	juce::String file_wildcard;
	std::vector<std::unique_ptr<Control>> controls;
	std::vector<std::unique_ptr<FileRow>> file_rows;
	std::unique_ptr<juce::FileChooser> chooser;
};
