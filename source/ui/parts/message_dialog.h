#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <vector>

struct MessageChoice
{
	juce::String text;
	std::function<void()> action;
};

class MessageDialog : public juce::Component
{
public:
	MessageDialog();

	void prepare(const juce::String& message, std::vector<MessageChoice> new_choices);
	void resized() override;

	std::function<void()> on_finished;

private:
	void choose(size_t index);

	juce::Label message_label;
	std::vector<MessageChoice> choices;
	std::vector<std::unique_ptr<juce::TextButton>> buttons;
};
