#pragma once

#include "core/app_context.h"
#include "platform/alsa_controls.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

class HardwareControlsView : public juce::Component
{
public:
	explicit HardwareControlsView(AppContext& context);

	void update();
	void resized() override;

private:
	struct ControlRow
	{
		juce::Label label;
		std::unique_ptr<juce::Component> editor;
	};

	void show_card(int card_index);
	void list_cards(const std::vector<AlsaCard>& cards, int selected_card);
	void build_rows();
	std::unique_ptr<juce::Component> make_editor(const AlsaControl& control);
	std::unique_ptr<juce::Component> make_choice_editor(const AlsaControl& control);
	std::unique_ptr<juce::Component> make_toggle_editor(const AlsaControl& control);
	std::unique_ptr<juce::Component> make_level_editor(const AlsaControl& control);
	void remember_value(const AlsaControl& control, int value);
	juce::String empty_message_text() const;

	AppContext& context;
	std::unique_ptr<AlsaControls> controls;
	juce::String card_name;
	int followed_card = NO_CARD;
	int followed_device_starts = -1;
	std::vector<std::unique_ptr<ControlRow>> rows;

	juce::Label title{{}, "Hardware controls"};
	juce::ComboBox card_box;
	juce::Label empty_message;
	juce::Viewport viewport;
	juce::Component row_container;
};
