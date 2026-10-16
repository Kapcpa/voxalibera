#pragma once

#include "engine/plugins/builtin/builtin_processor.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

class BuiltinEditor : public juce::Component
{
public:
	BuiltinEditor(BuiltinProcessor& processor, const juce::String& title);

	void paint(juce::Graphics& graphics) override;
	void resized() override;

private:
	struct Knob
	{
		juce::Label name;
		juce::Slider slider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
	};

	void add_knob(BuiltinParameter& parameter);
	int column_count() const;

	BuiltinProcessor& processor;
	juce::String title;
	std::vector<std::unique_ptr<Knob>> knobs;
};
