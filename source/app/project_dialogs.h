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
	void clean_up(Next next = {});
	void close_project_then(Next next);
	void check_recovery();

private:
	void open_file(const juce::File& file);
	void load(const juce::File& file, bool restore_autosave);
	void offer_restore(const juce::File& file, juce::Time autosaved, Next not_now);
	void offer_reopen(const juce::File& file);
	void offer_unsaved(std::vector<UnsavedSession> sessions, size_t index);
	void confirm_clean_up(const juce::Array<juce::File>& unused, Next next);
	void hint_cleanup_then(Next next);
	void show_problem(const juce::String& title, const juce::String& message);

	AppContext& context;
	ShowMessage show_message;
	std::unique_ptr<juce::FileChooser> chooser;
};
