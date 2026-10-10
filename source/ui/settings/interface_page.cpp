#include "ui/settings/interface_page.h"

#include "ui/settings/settings_parts.h"

namespace
{
	constexpr int EMPTY_MESSAGE_HEIGHT = 40;
	constexpr int CONTROL_LABEL_WIDTH = 190;
	constexpr int CONTROL_ROW_HEIGHT = 36;
	constexpr int VALUE_BOX_WIDTH = 50;
	constexpr int VALUE_BOX_HEIGHT = 20;

	int card_id(int card_index)
	{
		return card_index + 1;
	}

	int card_from_id(int id)
	{
		return id - 1;
	}
}

InterfacePage::InterfacePage(AppContext& context) : context(context)
{
	card_box.setTextWhenNothingSelected("No card");
	card_box.onChange = [this] { show_card(card_from_id(card_box.getSelectedId())); };
	empty_message.setJustificationType(juce::Justification::topLeft);
	viewport.setViewedComponent(&row_container, false);
	viewport.setScrollBarsShown(true, false);

	addAndMakeVisible(title);
	addAndMakeVisible(card_label);
	addAndMakeVisible(card_box);
	addAndMakeVisible(controls_title);
	addAndMakeVisible(empty_message);
	addAndMakeVisible(viewport);
}

void InterfacePage::update()
{
	const auto device_starts = context.engine.device_starts.load();

	if (context.devices.active_card == followed_card && device_starts == followed_device_starts)
		return;

	followed_card = context.devices.active_card;
	followed_device_starts = device_starts;
	show_card(followed_card);
}

void InterfacePage::show_card(int card_index)
{
	rows.clear();

	const auto cards = list_alsa_cards();
	controls = std::make_unique<AlsaControls>(card_index);
	card_name = find_card_name(cards, card_index);

	list_cards(cards, card_index);
	build_rows();
}

void InterfacePage::list_cards(const std::vector<AlsaCard>& cards, int selected_card)
{
	card_box.clear(juce::dontSendNotification);

	for (const auto& card : cards)
		card_box.addItem(juce::String(card.index) + ": " + card.name, card_id(card.index));

	card_box.setSelectedId(card_id(selected_card), juce::dontSendNotification);
}

void InterfacePage::build_rows()
{
	for (const auto& control : controls->controls) {
		auto row = std::make_unique<ControlRow>();
		row->label.setText(control.key, juce::dontSendNotification);
		row->label.setTooltip(control.key);
		row->editor = make_editor(control);

		row_container.addAndMakeVisible(row->label);
		row_container.addAndMakeVisible(*row->editor);
		rows.push_back(std::move(row));
	}

	empty_message.setText(empty_message_text(), juce::dontSendNotification);
	resized();
}

std::unique_ptr<juce::Component> InterfacePage::make_editor(const AlsaControl& control)
{
	switch (control.kind) {
		case AlsaControlKind::choice:
			return make_choice_editor(control);
		case AlsaControlKind::toggle:
			return make_toggle_editor(control);
		case AlsaControlKind::level:
			return make_level_editor(control);
	}

	return make_level_editor(control);
}

std::unique_ptr<juce::Component> InterfacePage::make_choice_editor(const AlsaControl& control)
{
	auto box = std::make_unique<juce::ComboBox>();
	box->addItemList(control.choices, 1);
	box->setSelectedId(controls->read(control) + 1, juce::dontSendNotification);
	box->onChange = [this, control, editor = box.get()] { remember_value(control, editor->getSelectedId() - 1); };
	return box;
}

std::unique_ptr<juce::Component> InterfacePage::make_toggle_editor(const AlsaControl& control)
{
	auto toggle = std::make_unique<juce::ToggleButton>();
	toggle->setToggleState(controls->read(control) != 0, juce::dontSendNotification);
	toggle->onClick = [this, control, editor = toggle.get()] { remember_value(control, editor->getToggleState() ? 1 : 0); };
	return toggle;
}

std::unique_ptr<juce::Component> InterfacePage::make_level_editor(const AlsaControl& control)
{
	auto slider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
	slider->setRange((double) control.minimum, (double) control.maximum, 1.0);
	slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, VALUE_BOX_WIDTH, VALUE_BOX_HEIGHT);
	slider->setValue(controls->read(control), juce::dontSendNotification);

	slider->onValueChange = [this, control, editor = slider.get()] {
		if (editor->isMouseButtonDown())
			controls->write(control, (int) editor->getValue());
		else
			remember_value(control, (int) editor->getValue());
	};

	slider->onDragEnd = [this, control, editor = slider.get()] { remember_value(control, (int) editor->getValue()); };
	return slider;
}

void InterfacePage::remember_value(const AlsaControl& control, int value)
{
	controls->write(control, value);
	context.settings.hardware_controls[card_name][control.key] = value;
	context.settings.save();
}

juce::String InterfacePage::empty_message_text() const
{
	if (!controls->is_open())
		return "No sound card matches the selected device. Pick one above to see its controls.";

	if (controls->controls.empty())
		return "This card has no adjustable controls.";

	return {};
}

void InterfacePage::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds();

	title.setBounds(area.removeFromTop(SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);

	auto card_row = area.removeFromTop(ROW_HEIGHT);
	card_label.setBounds(card_row.removeFromLeft(LABEL_WIDTH));
	card_box.setBounds(card_row.removeFromLeft(FIELD_WIDTH));
	area.removeFromTop(SECTION_GAP);

	controls_title.setBounds(area.removeFromTop(SECTION_LABEL_HEIGHT));
	area.removeFromTop(ROW_GAP);
	empty_message.setBounds(area.removeFromTop(empty_message.getText().isEmpty() ? 0 : EMPTY_MESSAGE_HEIGHT));
	viewport.setBounds(area);

	const auto width = viewport.getMaximumVisibleWidth();
	row_container.setSize(width, (int) rows.size() * CONTROL_ROW_HEIGHT);

	for (size_t position = 0; position < rows.size(); ++position) {
		auto bounds = juce::Rectangle<int>(0, (int) position * CONTROL_ROW_HEIGHT, width, ROW_HEIGHT);
		rows[position]->label.setBounds(bounds.removeFromLeft(CONTROL_LABEL_WIDTH));
		rows[position]->editor->setBounds(bounds.removeFromLeft(FIELD_WIDTH));
	}
}
