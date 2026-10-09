#include "ui/hardware_controls_view.h"

namespace
{
constexpr int TITLE_HEIGHT = 24;
constexpr int CARD_BOX_HEIGHT = 26;
constexpr int EMPTY_MESSAGE_HEIGHT = 40;
constexpr int ROW_HEIGHT = 30;
constexpr int ROW_PADDING = 2;
constexpr int LABEL_WIDTH = 170;
constexpr int VALUE_BOX_WIDTH = 50;
constexpr int VALUE_BOX_HEIGHT = 20;
constexpr int SECTION_GAP = 8;
constexpr float TITLE_FONT_SIZE = 16.0f;

int card_id(int card_index)
{
	return card_index + 1;
}

int card_from_id(int id)
{
	return id - 1;
}
}

HardwareControlsView::HardwareControlsView(AppContext& context)
	: context(context)
{
	title.setFont(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold));
	card_box.setTextWhenNothingSelected("No card");
	card_box.onChange = [this]
	{ show_card(card_from_id(card_box.getSelectedId())); };
	empty_message.setJustificationType(juce::Justification::topLeft);
	viewport.setViewedComponent(&row_container, false);
	viewport.setScrollBarsShown(true, false);

	addAndMakeVisible(title);
	addAndMakeVisible(card_box);
	addAndMakeVisible(empty_message);
	addAndMakeVisible(viewport);
}

void HardwareControlsView::update()
{
	const auto device_starts = context.engine.device_starts.load();

	if (context.devices.active_card == followed_card && device_starts == followed_device_starts)
		return;

	followed_card = context.devices.active_card;
	followed_device_starts = device_starts;
	show_card(followed_card);
}

void HardwareControlsView::show_card(int card_index)
{
	rows.clear();

	const auto cards = list_alsa_cards();
	controls = std::make_unique<AlsaControls>(card_index);
	card_name = find_card_name(cards, card_index);

	list_cards(cards, card_index);
	apply_saved_values();
	build_rows();
}

void HardwareControlsView::list_cards(const std::vector<AlsaCard>& cards, int selected_card)
{
	card_box.clear(juce::dontSendNotification);

	for (const auto& card : cards)
		card_box.addItem(juce::String(card.index) + ": " + card.name, card_id(card.index));

	card_box.setSelectedId(card_id(selected_card), juce::dontSendNotification);
}

void HardwareControlsView::apply_saved_values()
{
	const auto saved_card = context.settings.hardware_controls.find(card_name);

	if (saved_card == context.settings.hardware_controls.end())
		return;

	for (const auto& control : controls->controls)
		if (const auto saved = saved_card->second.find(control.key); saved != saved_card->second.end())
			controls->write(control, saved->second);
}

void HardwareControlsView::build_rows()
{
	for (const auto& control : controls->controls)
	{
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

std::unique_ptr<juce::Component> HardwareControlsView::make_editor(const AlsaControl& control)
{
	switch (control.kind)
	{
		case AlsaControlKind::choice:
			return make_choice_editor(control);
		case AlsaControlKind::toggle:
			return make_toggle_editor(control);
		case AlsaControlKind::level:
			return make_level_editor(control);
	}

	return make_level_editor(control);
}

std::unique_ptr<juce::Component> HardwareControlsView::make_choice_editor(const AlsaControl& control)
{
	auto box = std::make_unique<juce::ComboBox>();
	box->addItemList(control.choices, 1);
	box->setSelectedId(controls->read(control) + 1, juce::dontSendNotification);
	box->onChange = [this, control, editor = box.get()]
	{ remember_value(control, editor->getSelectedId() - 1); };
	return box;
}

std::unique_ptr<juce::Component> HardwareControlsView::make_toggle_editor(const AlsaControl& control)
{
	auto toggle = std::make_unique<juce::ToggleButton>();
	toggle->setToggleState(controls->read(control) != 0, juce::dontSendNotification);
	toggle->onClick = [this, control, editor = toggle.get()]
	{ remember_value(control, editor->getToggleState() ? 1 : 0); };
	return toggle;
}

std::unique_ptr<juce::Component> HardwareControlsView::make_level_editor(const AlsaControl& control)
{
	auto slider = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight);
	slider->setRange((double) control.minimum, (double) control.maximum, 1.0);
	slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, VALUE_BOX_WIDTH, VALUE_BOX_HEIGHT);
	slider->setValue(controls->read(control), juce::dontSendNotification);

	slider->onValueChange = [this, control, editor = slider.get()]
	{
		if (editor->isMouseButtonDown())
			controls->write(control, (int) editor->getValue());
		else
			remember_value(control, (int) editor->getValue());
	};

	slider->onDragEnd = [this, control, editor = slider.get()]
	{ remember_value(control, (int) editor->getValue()); };
	return slider;
}

void HardwareControlsView::remember_value(const AlsaControl& control, int value)
{
	controls->write(control, value);
	context.settings.hardware_controls[card_name][control.key] = value;
	context.settings.save();
}

juce::String HardwareControlsView::empty_message_text() const
{
	if (!controls->is_open())
		return "No sound card matches the selected device. Pick one above.";

	if (controls->controls.empty())
		return "This card has no adjustable controls.";

	return {};
}

void HardwareControlsView::resized()
{
	auto area = getLocalBounds();

	title.setBounds(area.removeFromTop(TITLE_HEIGHT));
	area.removeFromTop(SECTION_GAP / 2);
	card_box.setBounds(area.removeFromTop(CARD_BOX_HEIGHT));
	area.removeFromTop(SECTION_GAP);
	empty_message.setBounds(area.removeFromTop(empty_message.getText().isEmpty() ? 0 : EMPTY_MESSAGE_HEIGHT));
	viewport.setBounds(area);

	const auto width = viewport.getMaximumVisibleWidth();
	row_container.setSize(width, (int) rows.size() * ROW_HEIGHT);

	for (size_t position = 0; position < rows.size(); ++position)
	{
		auto bounds = juce::Rectangle<int>(0, (int) position * ROW_HEIGHT, width, ROW_HEIGHT).reduced(0, ROW_PADDING);
		rows[position]->label.setBounds(bounds.removeFromLeft(LABEL_WIDTH));
		rows[position]->editor->setBounds(bounds);
	}
}
