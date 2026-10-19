#include "ui/plugins/builtin_editor.h"

#include "ui/theme.h"

#include <algorithm>

namespace
{
	constexpr int MAX_COLUMNS = 4;
	constexpr int MIN_WIDTH = 260;
	constexpr int HEADER_HEIGHT = 40;
	constexpr int PADDING = 16;
	constexpr int KNOB_WIDTH = 88;
	constexpr int KNOB_GAP = 8;
	constexpr int NAME_HEIGHT = 18;
	constexpr int DIAL_HEIGHT = 64;
	constexpr int VALUE_HEIGHT = 20;
	constexpr int KNOB_HEIGHT = NAME_HEIGHT + DIAL_HEIGHT + VALUE_HEIGHT;
	constexpr int CHOICE_HEIGHT = 26;
	constexpr int FILE_ROW_HEIGHT = 28;
	constexpr int FILE_ROW_GAP = 6;
	constexpr int FILE_NAME_WIDTH = 52;
	constexpr int FILE_BUTTON_WIDTH = 64;
	constexpr int FILE_MIN_WIDTH = 420;
	constexpr float TITLE_FONT_SIZE = 15.0f;
	constexpr float NAME_FONT_SIZE = 12.0f;
	constexpr float PANEL_CORNER_RADIUS = 6.0f;
	constexpr auto EMPTY_TEXT = "This effect has no settings";
	constexpr auto MISSING_SUFFIX = " (missing)";

	void style_name(juce::Label& label, const juce::String& text)
	{
		label.setText(text, juce::dontSendNotification);
		label.setFont(juce::FontOptions(NAME_FONT_SIZE, juce::Font::bold));
		label.setColour(juce::Label::textColourId, theme::DIM_TEXT.brighter(0.5f));
	}
}

BuiltinEditor::BuiltinEditor(BuiltinProcessor& processor, const juce::String& title, FileImport import_file, juce::String file_wildcard) : processor(processor), title(title), import_file(std::move(import_file)), file_wildcard(std::move(file_wildcard))
{
	for (const auto& file : processor.files())
		add_file_row(*file);

	for (const auto& parameter : processor.parameters())
		add_control(*parameter);

	const auto columns = column_count();
	const auto rows = std::max(1, ((int) controls.size() + columns - 1) / columns);
	const auto files_width = file_rows.empty() ? 0 : FILE_MIN_WIDTH;
	const auto width = std::max({MIN_WIDTH, files_width, columns * KNOB_WIDTH + (columns - 1) * KNOB_GAP + 4 * PADDING});
	const auto height = HEADER_HEIGHT + files_height() + rows * KNOB_HEIGHT + (rows - 1) * KNOB_GAP + 4 * PADDING;
	setSize(width, height);
}

void BuiltinEditor::add_control(BuiltinParameter& parameter)
{
	auto& control = *controls.emplace_back(std::make_unique<Control>());
	style_name(control.name, parameter.info.name);
	control.name.setJustificationType(juce::Justification::centred);
	addAndMakeVisible(control.name);

	if (parameter.is_choice())
		add_choice(control, parameter);
	else
		add_slider(control, parameter);
}

void BuiltinEditor::add_slider(Control& control, BuiltinParameter& parameter)
{
	const auto& info = parameter.info;
	auto& slider = *(control.slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow));

	slider.setRange(info.minimum, info.maximum);

	if (info.centre)
		slider.setSkewFactorFromMidPoint(*info.centre);

	slider.setValue(parameter.value(), juce::dontSendNotification);
	slider.setDoubleClickReturnValue(true, info.default_value);
	slider.setNumDecimalPlacesToDisplay(info.decimals);
	slider.setTextValueSuffix(info.unit.isEmpty() ? juce::String() : " " + info.unit);
	slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, KNOB_WIDTH, VALUE_HEIGHT);
	slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
	slider.setScrollWheelEnabled(false);
	slider.onValueChange = [this, &slider, &parameter] { processor.set_parameter(parameter, (float) slider.getValue()); };
	addAndMakeVisible(slider);
}

void BuiltinEditor::add_choice(Control& control, BuiltinParameter& parameter)
{
	auto& choice = *(control.choice = std::make_unique<juce::ComboBox>());
	choice.addItemList(parameter.info.choices, 1);
	choice.setSelectedItemIndex(juce::roundToInt(parameter.value()), juce::dontSendNotification);
	choice.onChange = [this, &choice, &parameter] { processor.set_parameter(parameter, (float) choice.getSelectedItemIndex()); };
	addAndMakeVisible(choice);
}

void BuiltinEditor::add_file_row(BuiltinFile& file)
{
	auto& row = *file_rows.emplace_back(std::make_unique<FileRow>(file));
	style_name(row.name, file.info.name);
	row.current.setColour(juce::Label::textColourId, theme::TEXT);
	row.load.onClick = [this, &row] { choose_file(row); };
	row.clear.onClick = [this, &row] {
		processor.set_file(row.file, {});
		show_file(row);
	};

	show_file(row);

	for (auto* component : std::initializer_list<juce::Component*>{&row.name, &row.current, &row.load, &row.clear})
		addAndMakeVisible(component);
}

void BuiltinEditor::choose_file(FileRow& row)
{
	chooser = std::make_unique<juce::FileChooser>("Load " + row.file.info.name, juce::File::getSpecialLocation(juce::File::userHomeDirectory), file_wildcard);
	chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this, &row](const juce::FileChooser& finished) {
		const auto chosen = finished.getResult();

		if (chosen == juce::File() || import_file == nullptr)
			return;

		if (const auto relative = import_file(chosen); relative.isNotEmpty()) {
			processor.set_file(row.file, relative);
			show_file(row);
		}
	});
}

void BuiltinEditor::show_file(FileRow& row)
{
	const auto& path = row.file.path();

	if (path.isEmpty()) {
		row.current.setText(row.file.info.empty_text, juce::dontSendNotification);
		row.current.setColour(juce::Label::textColourId, theme::DIM_TEXT.brighter(0.5f));
		row.clear.setEnabled(false);
		return;
	}

	const auto file = processor.resolve(row.file);
	const auto missing = !file.existsAsFile();
	row.current.setText(juce::File::createFileWithoutCheckingPath(path).getFileNameWithoutExtension() + (missing ? MISSING_SUFFIX : ""), juce::dontSendNotification);
	row.current.setColour(juce::Label::textColourId, missing ? theme::RECORD : theme::TEXT);
	row.clear.setEnabled(true);
}

int BuiltinEditor::column_count() const
{
	return std::clamp((int) controls.size(), 1, MAX_COLUMNS);
}

int BuiltinEditor::files_height() const
{
	return file_rows.empty() ? 0 : (int) file_rows.size() * (FILE_ROW_HEIGHT + FILE_ROW_GAP) + PADDING;
}

void BuiltinEditor::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);

	auto area = getLocalBounds();
	const auto header = area.removeFromTop(HEADER_HEIGHT);

	graphics.setColour(theme::PANEL_BACKGROUND);
	graphics.fillRect(header);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawHorizontalLine(header.getBottom() - 1, 0.0f, (float) getWidth());
	graphics.setColour(theme::TEXT);
	graphics.setFont(juce::FontOptions(TITLE_FONT_SIZE, juce::Font::bold));
	graphics.drawText(title, header.reduced(PADDING, 0), juce::Justification::centredLeft, true);

	const auto panel = area.reduced(PADDING).toFloat();
	graphics.setColour(theme::PANEL_BACKGROUND);
	graphics.fillRoundedRectangle(panel, PANEL_CORNER_RADIUS);
	graphics.setColour(theme::PANEL_BORDER);
	graphics.drawRoundedRectangle(panel, PANEL_CORNER_RADIUS, 1.0f);

	if (!controls.empty() || !file_rows.empty())
		return;

	graphics.setColour(theme::DIM_TEXT);
	graphics.setFont(juce::FontOptions(NAME_FONT_SIZE));
	graphics.drawText(EMPTY_TEXT, panel, juce::Justification::centred, false);
}

void BuiltinEditor::resized()
{
	auto content = getLocalBounds().withTrimmedTop(HEADER_HEIGHT).reduced(2 * PADDING);

	for (auto& row : file_rows) {
		auto line = content.removeFromTop(FILE_ROW_HEIGHT);
		content.removeFromTop(FILE_ROW_GAP);
		row->name.setBounds(line.removeFromLeft(FILE_NAME_WIDTH));
		row->clear.setBounds(line.removeFromRight(FILE_BUTTON_WIDTH));
		line.removeFromRight(KNOB_GAP);
		row->load.setBounds(line.removeFromRight(FILE_BUTTON_WIDTH));
		line.removeFromRight(KNOB_GAP);
		row->current.setBounds(line);
	}

	if (!file_rows.empty())
		content.removeFromTop(PADDING);

	const auto columns = column_count();
	const auto row_width = columns * KNOB_WIDTH + (columns - 1) * KNOB_GAP;
	const auto left = content.getX() + (content.getWidth() - row_width) / 2;

	for (size_t index = 0; index < controls.size(); ++index) {
		const auto column = (int) index % columns;
		const auto row = (int) index / columns;
		auto cell = juce::Rectangle<int>(left + column * (KNOB_WIDTH + KNOB_GAP), content.getY() + row * (KNOB_HEIGHT + KNOB_GAP), KNOB_WIDTH, KNOB_HEIGHT);
		auto& control = *controls[index];

		control.name.setBounds(cell.removeFromTop(NAME_HEIGHT));

		if (control.slider != nullptr)
			control.slider->setBounds(cell);
		else
			control.choice->setBounds(cell.withSizeKeepingCentre(KNOB_WIDTH, CHOICE_HEIGHT));
	}
}
