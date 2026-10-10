#pragma once

#include "core/app_context.h"
#include "ui/message_dialog.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

class ProjectDialogs
{
public:
	using Next = std::function<void()>;
	using ShowMessage = std::function<void(const juce::String& title, const juce::String& message, std::vector<MessageChoice> choices)>;

	ProjectDialogs(AppContext& context, ShowMessage show_message);

	void new_project();
	void open_project();
	void open_recent(const juce::File& file);
	void save(Next next = {});
	void save_as(Next next = {});
	void close_project_then(Next next);

private:
	void open_file(const juce::File& file);
	void show_problem(const juce::String& title, const juce::String& message);

	AppContext& context;
	ShowMessage show_message;
	std::unique_ptr<juce::FileChooser> chooser;
};
