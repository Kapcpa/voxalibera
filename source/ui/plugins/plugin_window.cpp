#include "ui/plugins/plugin_window.h"

#include "ui/theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace
{
	bool is_resizable(const juce::Component& editor)
	{
		const auto* plugin_editor = dynamic_cast<const juce::AudioProcessorEditor*>(&editor);
		return plugin_editor != nullptr && plugin_editor->isResizable();
	}
}

PluginWindow::PluginWindow(const juce::String& title, std::shared_ptr<PluginProcessor> processor, std::unique_ptr<juce::Component> editor) : DocumentWindow(title, theme::WINDOW_BACKGROUND, closeButton), processor(std::move(processor)), editor(std::move(editor))
{
	setUsingNativeTitleBar(true);
	setContentNonOwned(this->editor.get(), true);
	setResizable(is_resizable(*this->editor), false);
	centreWithSize(getWidth(), getHeight());
	setVisible(true);
}

PluginWindow::~PluginWindow()
{
	clearContentComponent();
}

void PluginWindow::closeButtonPressed()
{
	setVisible(false);
	closed = true;
}

PluginWindows::PluginWindows(AppContext& context) : context(context)
{
}

void PluginWindows::open(PluginId id)
{
	const auto processor = context.plugins.find_instance(id);
	const auto* plugin = context.project.find_plugin(id);
	const auto* track = context.project.find_track_of(id);

	if (processor == nullptr || plugin == nullptr || track == nullptr)
		return;

	if (const auto found = windows.find(id); found != windows.end() && found->second->processor == processor && !found->second->closed) {
		found->second->toFront(true);
		return;
	}

	auto editor = processor->create_editor();

	if (editor == nullptr)
		return;

	windows[id] = std::make_unique<PluginWindow>(plugin->name + " - " + track->name, processor, std::move(editor));
}

void PluginWindows::update()
{
	std::erase_if(windows, [this](const auto& entry) { return entry.second->closed || context.plugins.find_instance(entry.first) != entry.second->processor; });
}
