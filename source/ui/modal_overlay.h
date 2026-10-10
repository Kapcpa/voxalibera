#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

class ModalOverlay : public juce::Component
{
public:
	ModalOverlay();

	void open(const juce::String& new_title, juce::Component& new_content, bool with_close_button = true);
	void close();
	bool is_open() const { return isVisible(); }

	void paint(juce::Graphics& graphics) override;
	void resized() override;
	bool keyPressed(const juce::KeyPress& key) override;

	std::function<void()> on_close;

private:
	juce::Rectangle<int> dialog_bounds() const;

	juce::String title;
	juce::Component* content = nullptr;
	juce::Rectangle<int> content_size;
	juce::TextButton close_button{"Close"};
};
