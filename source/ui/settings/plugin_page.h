#pragma once

#include "core/app_context.h"
#include "ui/settings/settings_parts.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

class PluginPage : public SettingsPage
{
public:
	explicit PluginPage(AppContext& context);

	void update() override;
	void resized() override;

	void add_plugin_folder();

private:
	struct FolderRow
	{
		juce::Label path_label;
		juce::TextButton remove_button{juce::CharPointer_UTF8("\xc3\x97")};
	};

	void start_scan(bool rescan_everything);
	juce::String describe_scan() const;
	void rebuild_folder_rows();

	AppContext& context;

	SectionLabel title{"Plugins"};
	juce::TextButton scan_button{"Scan for new"};
	juce::TextButton rescan_button{"Rescan all"};
	juce::Label scan_status;
	int plugins_before_scan = -1;

	SectionLabel folder_label{"Folders"};
	juce::Label default_folders_label;
	std::vector<std::unique_ptr<FolderRow>> folder_rows;
	juce::TextButton add_plugin_folder_button{"Add plugin folder"};

	std::unique_ptr<juce::FileChooser> file_chooser;
};
