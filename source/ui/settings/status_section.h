#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

class StatusSection : public juce::Component
{
public:
	explicit StatusSection(AppContext& context);

	void update();
	void resized() override;

	static constexpr int ITEM_COUNT = 10;

	static int preferred_height();

private:
	struct Item
	{
		juce::Label name;
		juce::Label value;
	};

	std::array<juce::String, ITEM_COUNT> describe() const;
	std::array<juce::String, ITEM_COUNT> describe_missing_device() const;

	AppContext& context;
	std::array<Item, ITEM_COUNT> items;
	int frames_until_refresh = 0;
};
