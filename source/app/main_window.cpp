#include "app/main_window.h"

#include "app/commands.h"
#include "ui/theme.h"

namespace
{
	constexpr int FRAMES_PER_SECOND = 30;
	constexpr int MIN_WIDTH = 1000;
	constexpr int MIN_HEIGHT = 700;
	constexpr int MAX_SIZE = 10000;
}

MainWindow::MainWindow(const juce::String& name, AppContext& context) : DocumentWindow(name, theme::WINDOW_BACKGROUND, allButtons), context(context), settings_view(context), settings_overlay("Settings", settings_view)
{
	actions = {
		{command::QUIT, [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }},
		{command::TOGGLE_MIXER, [this] { toggle_mixer(); }},
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

	settings_overlay.on_close = [this] { command_manager.commandStatusChanged(); };

	setVisible(true);

	startTimerHz(FRAMES_PER_SECOND);
}

MainWindow::~MainWindow()
{
	stopTimer();
	clearContentComponent();
}

void MainWindow::closeButtonPressed()
{
	juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::timerCallback()
{
	context.update();

	if (settings_overlay.is_open())
		settings_view.update();
}

bool MainWindow::is_allowed_now(juce::CommandID id) const
{
	return !settings_overlay.is_open() || id == command::QUIT;
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
	info.setActive(actions.contains(id) && is_allowed_now(id));

	if (definition->key != 0)
		info.addDefaultKeypress(definition->key, definition->modifiers);

	if (id == command::TOGGLE_MIXER)
		info.setTicked(main_view.is_mixer_visible());
}

bool MainWindow::perform(const InvocationInfo& info)
{
	const auto action = actions.find(info.commandID);

	if (action == actions.end() || !is_allowed_now(info.commandID))
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
	settings_overlay.open();
	command_manager.commandStatusChanged();
}

void MainWindow::show_about()
{
	const auto options = juce::MessageBoxOptions()
							 .withIconType(juce::MessageBoxIconType::InfoIcon)
							 .withTitle(getName())
							 .withMessage("Version " + juce::JUCEApplication::getInstance()->getApplicationVersion() + "\nOpen-source DAW for Linux, AGPLv3.")
							 .withButton("OK");

	juce::NativeMessageBox::showAsync(options, nullptr);
}
