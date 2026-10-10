#include "app/main_window.h"

#include "app/commands.h"
#include "ui/theme.h"

namespace
{
	juce::String describe_deletion(const Selection& selection)
	{
		if (!selection.clips.empty())
			return selection.clips.size() == 1 ? "Delete Clip" : "Delete Clips";

		if (!selection.tracks.empty())
			return selection.tracks.size() == 1 ? "Delete Track" : "Delete Tracks";

		return "Delete";
	}

	constexpr int FRAMES_PER_SECOND = 30;
	constexpr int MIN_WIDTH = 1000;
	constexpr int MIN_HEIGHT = 700;
	constexpr int MAX_SIZE = 10000;
	const juce::String UNSAVED_MARK = juce::String::fromUTF8(" \xe2\x80\xa2");
}

WindowContent::WindowContent(juce::MenuBarModel& menu, MainView& main_view, ModalOverlay& overlay) : menu_bar(&menu), main_view(main_view), overlay(overlay)
{
	addAndMakeVisible(menu_bar);
	addAndMakeVisible(main_view);
	addChildComponent(overlay);
	setSize(main_view.getWidth(), main_view.getHeight() + getLookAndFeel().getDefaultMenuBarHeight());
}

void WindowContent::resized()
{
	auto area = getLocalBounds();
	overlay.setBounds(area);
	menu_bar.setBounds(area.removeFromTop(getLookAndFeel().getDefaultMenuBarHeight()));
	main_view.setBounds(area);
}

MainWindow::MainWindow(const juce::String& name, AppContext& context) : DocumentWindow(name, theme::WINDOW_BACKGROUND, allButtons), context(context), main_view(context), settings_view(context), new_track_dialog(context)
{
	actions = {
		{command::NEW_PROJECT, [this] { project_dialogs.new_project(); }},
		{command::OPEN_PROJECT, [this] { project_dialogs.open_project(); }},
		{command::SAVE_PROJECT, [this] { project_dialogs.save(); }},
		{command::SAVE_PROJECT_AS, [this] { project_dialogs.save_as(); }},
		{command::CLEAN_UP_PROJECT, [this] { project_dialogs.clean_up(); }},
		{command::QUIT, [this] { request_quit(); }},
		{command::TOGGLE_MIXER, [this] { toggle_mixer(); }},
		{command::PLAY_STOP, [this] { this->context.toggle_playback(); }},
		{command::RETURN_TO_START, [this] { this->context.return_to_start(); }},
		{command::TOGGLE_METRONOME, [this] { this->context.toggle_metronome(); }},
		{command::UNDO, [this] { this->context.undo(); }},
		{command::REDO, [this] { this->context.redo(); }},
		{command::NEW_TRACK, [this] { show_new_track_dialog(); }},
		{command::DELETE_SELECTION, [this] { this->context.delete_selection(); }},
		{command::RECORD, [this] { this->context.toggle_recording(); }},
		{command::ARM_TRACKS, [this] { this->context.tracks.toggle_selected(&Track::armed, "Arm Track"); }},
		{command::MUTE_TRACKS, [this] { this->context.tracks.toggle_selected(&Track::mute, "Mute Track"); }},
		{command::SOLO_TRACKS, [this] { this->context.tracks.toggle_selected(&Track::solo, "Solo Track"); }},
		{command::AUDIO_SETTINGS, [this] { show_settings(SettingsView::AUDIO_PAGE); }},
		{command::INTERFACE_SETTINGS, [this] { show_settings(SettingsView::INTERFACE_PAGE); }},
		{command::ABOUT, [this] { show_about(); }},
	};

	command_manager.registerAllCommandsForTarget(this);
	command_manager.setFirstCommandTarget(this);
	addKeyListener(command_manager.getKeyMappings());

	setUsingNativeTitleBar(true);
	setContentNonOwned(&content, true);
	setResizable(true, true);
	setResizeLimits(MIN_WIDTH, MIN_HEIGHT, MAX_SIZE, MAX_SIZE);
	centreWithSize(getWidth(), getHeight());

	overlay.on_close = [this] { command_manager.commandStatusChanged(); };
	new_track_dialog.on_finished = [this] { close_overlay(); };
	message_dialog.on_finished = [this] { close_overlay(); };
	main_view.track_area.on_add_track = [this] { show_new_track_dialog(); };
	main_menu.recent_files = [this] { return this->context.session.recent.files; };
	main_menu.can_open_recent = [this] { return is_allowed_now(command::OPEN_PROJECT) && is_available(command::OPEN_PROJECT); };
	main_menu.on_open_recent = [this](const juce::File& file) { project_dialogs.open_recent(file); };

	setVisible(true);
	juce::MessageManager::callAsync([window = juce::Component::SafePointer<MainWindow>(this)] {
		if (window != nullptr)
			window->project_dialogs.check_recovery();
	});

	startTimerHz(FRAMES_PER_SECOND);
}

MainWindow::~MainWindow()
{
	stopTimer();
	clearContentComponent();
}

void MainWindow::closeButtonPressed()
{
	request_quit();
}

void MainWindow::request_quit()
{
	if (context.recorder.is_recording()) {
		context.recorder.stop();
		quit_pending = true;
		return;
	}

	close_overlay();
	project_dialogs.close_project_then([] { juce::JUCEApplication::getInstance()->quit(); });
}

void MainWindow::quit_after_recording()
{
	if (!quit_pending || context.recorder.is_recording())
		return;

	quit_pending = false;
	request_quit();
}

void MainWindow::show_title()
{
	const auto title = context.session.name() + (context.session.has_unsaved_changes() ? UNSAVED_MARK : juce::String()) + " - " + juce::JUCEApplication::getInstance()->getApplicationName();

	if (getName() != title)
		setName(title);
}

void MainWindow::timerCallback()
{
	context.update();
	main_view.update();
	show_title();
	quit_after_recording();
	release_focus_outside_text_fields();

	if (overlay.is_open())
		settings_view.update();
}

void MainWindow::release_focus_outside_text_fields()
{
	auto* focused = juce::Component::getCurrentlyFocusedComponent();

	if (focused == nullptr || !main_view.isParentOf(focused) || dynamic_cast<juce::TextEditor*>(focused) != nullptr)
		return;

	focused->giveAwayKeyboardFocus();
}

bool MainWindow::is_allowed_now(juce::CommandID id) const
{
	return !overlay.is_open() || id == command::QUIT;
}

bool MainWindow::is_available(juce::CommandID id) const
{
	switch (id) {
		case command::UNDO:
			return context.edits.can_undo();
		case command::REDO:
			return context.edits.can_redo();
		case command::NEW_PROJECT:
		case command::OPEN_PROJECT:
		case command::SAVE_PROJECT:
		case command::SAVE_PROJECT_AS:
		case command::CLEAN_UP_PROJECT:
			return !context.recorder.is_recording();
		case command::DELETE_SELECTION:
			return !context.selection.tracks.empty() || !context.selection.clips.empty();
		case command::ARM_TRACKS:
		case command::MUTE_TRACKS:
		case command::SOLO_TRACKS:
			return !context.selection.tracks.empty();
		default:
			return true;
	}
}

void MainWindow::describe_dynamic_command(juce::CommandID id, juce::ApplicationCommandInfo& info) const
{
	if (id == command::UNDO && context.edits.can_undo())
		info.shortName = "Undo " + context.edits.undo_name();

	if (id == command::REDO && context.edits.can_redo())
		info.shortName = "Redo " + context.edits.redo_name();

	if (id == command::DELETE_SELECTION)
		info.shortName = describe_deletion(context.selection);

	if (id == command::TOGGLE_MIXER)
		info.setTicked(main_view.is_mixer_visible());

	if (id == command::TOGGLE_METRONOME)
		info.setTicked(context.project.metronome);
}

juce::ApplicationCommandTarget* MainWindow::getNextCommandTarget()
{
	return nullptr;
}

void MainWindow::getAllCommands(juce::Array<juce::CommandID>& commands)
{
	for (const auto& definition : command_definitions())
		commands.add(definition.id);
}

void MainWindow::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& info)
{
	const auto* definition = find_command(id);

	if (definition == nullptr)
		return;

	info.setInfo(definition->name, definition->name, definition->category, juce::ApplicationCommandInfo::dontTriggerVisualFeedback);
	info.setActive(actions.contains(id) && is_allowed_now(id) && is_available(id));

	if (definition->key != 0)
		info.addDefaultKeypress(definition->key, definition->modifiers);

	describe_dynamic_command(id, info);
}

bool MainWindow::perform(const InvocationInfo& info)
{
	const auto action = actions.find(info.commandID);

	if (action == actions.end() || !is_allowed_now(info.commandID) || !is_available(info.commandID))
		return false;

	action->second();
	return true;
}

void MainWindow::toggle_mixer()
{
	main_view.set_mixer_visible(!main_view.is_mixer_visible());
	command_manager.commandStatusChanged();
}

void MainWindow::show_settings(int page)
{
	settings_view.show_page(page);
	overlay.open("Settings", settings_view);
	command_manager.commandStatusChanged();
}

void MainWindow::show_new_track_dialog()
{
	overlay.open("New Track", new_track_dialog);
	new_track_dialog.prepare();
	command_manager.commandStatusChanged();
}

void MainWindow::close_overlay()
{
	overlay.close();
}

void MainWindow::show_about()
{
	show_message("About " + juce::JUCEApplication::getInstance()->getApplicationName(), "Version " + juce::JUCEApplication::getInstance()->getApplicationVersion() + "\nOpen-source DAW for Linux, AGPLv3.", {{"OK", {}}});
}

void MainWindow::show_message(const juce::String& title, const juce::String& message, std::vector<MessageChoice> choices)
{
	message_dialog.prepare(message, std::move(choices));
	overlay.open(title, message_dialog, false);
	command_manager.commandStatusChanged();
}
