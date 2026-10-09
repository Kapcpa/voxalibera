#include "app/main_window.h"

#include "app/commands.h"
#include "ui/theme.h"

namespace
{
constexpr int FRAMES_PER_SECOND = 30;
}

MainWindow::MainWindow(const juce::String& name, AppContext& context)
	: DocumentWindow(name, theme::WINDOW_BACKGROUND, allButtons),
	  context(context),
	  settings_window(context)
{
	actions = {
		{command::QUIT, [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }},
		{command::TOGGLE_MIXER, [this] { toggle_mixer(); }},
		{command::AUDIO_SETTINGS, [this] { show_audio_settings(); }},
		{command::ABOUT, [this] { show_about(); }}
	};

	command_manager.registerAllCommandsForTarget(this);
	command_manager.setFirstCommandTarget(this);
	addKeyListener(command_manager.getKeyMappings());

	setUsingNativeTitleBar(true);
	setMenuBar(&main_menu);
	setContentNonOwned(&main_view, true);
	setResizable(true, true);
	centreWithSize(getWidth(), getHeight());
	setVisible(true);

	startTimerHz(FRAMES_PER_SECOND);
}

MainWindow::~MainWindow()
{
	stopTimer();
	setMenuBar(nullptr);
	clearContentComponent();
}

void MainWindow::closeButtonPressed()
{
	juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainWindow::timerCallback()
{
	context.update();
	settings_window.update();
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

	info.setInfo(definition->name, definition->name, definition->category, 0);
	info.setActive(actions.contains(id));

	if (definition->key != 0)
		info.addDefaultKeypress(definition->key, definition->modifiers);

	if (id == command::TOGGLE_MIXER)
		info.setTicked(main_view.is_mixer_visible());
}

bool MainWindow::perform(const InvocationInfo& info)
{
	const auto action = actions.find(info.commandID);

	if (action == actions.end())
		return false;

	action->second();
	return true;
}

void MainWindow::toggle_mixer()
{
	main_view.set_mixer_visible(!main_view.is_mixer_visible());
	command_manager.commandStatusChanged();
}

void MainWindow::show_audio_settings()
{
	settings_window.setVisible(true);
	settings_window.toFront(true);
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
