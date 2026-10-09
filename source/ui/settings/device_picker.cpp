#include "ui/settings/device_picker.h"

#include "ui/settings/settings_layout.h"

namespace
{
	const std::vector<int> JACK_BUFFER_SIZES{32, 64, 128, 256, 512, 1024};
	constexpr int LARGEST_BUFFER_SIZE = 4096;
	constexpr int NOTHING_SELECTED = -1;

	bool is_power_of_two(int value)
	{
		return value > 0 && (value & (value - 1)) == 0;
	}

	juce::String describe_buffer(int size, double sample_rate)
	{
		const auto milliseconds = sample_rate > 0.0 ? juce::String(1000.0 * size / sample_rate, 1) + " ms" : juce::String();
		return juce::String(size) + " samples" + (milliseconds.isEmpty() ? "" : "  (" + milliseconds + ")");
	}

	void fill(juce::ComboBox& box, const juce::StringArray& items, int selected_index)
	{
		box.clear(juce::dontSendNotification);
		box.addItemList(items, 1);
		box.setSelectedItemIndex(selected_index, juce::dontSendNotification);
		box.setEnabled(!items.isEmpty());
	}

	template <typename Value>
	int index_of(const std::vector<Value>& values, Value value)
	{
		const auto found = std::find(values.begin(), values.end(), value);
		return found == values.end() ? NOTHING_SELECTED : (int) std::distance(values.begin(), found);
	}
}

DevicePicker::DevicePicker(AppContext& context) : context(context), device_manager(context.engine.device_manager)
{
	driver.label.setText("Driver", juce::dontSendNotification);
	output.label.setText("Output", juce::dontSendNotification);
	input.label.setText("Input", juce::dontSendNotification);
	sample_rate.label.setText("Sample rate", juce::dontSendNotification);
	buffer_size.label.setText("Buffer", juce::dontSendNotification);

	driver.box.onChange = [this] { choose_driver(); };
	output.box.onChange = [this] { choose_output(); };
	input.box.onChange = [this] { choose_input(); };
	sample_rate.box.onChange = [this] { choose_sample_rate(); };
	buffer_size.box.onChange = [this] { choose_buffer_size(); };
	rescan_button.onClick = [this] { rescan(); };

	for (auto* row : rows) {
		row->box.setTextWhenNothingSelected("None");
		row->box.setTextWhenNoChoicesAvailable("None");
		addAndMakeVisible(row->label);
		addAndMakeVisible(row->box);
	}

	addAndMakeVisible(rescan_button);
	rebuild();
}

int DevicePicker::preferred_height()
{
	constexpr int ROWS_INCLUDING_RESCAN = 6;
	return ROWS_INCLUDING_RESCAN * (settings_layout::ROW_HEIGHT + settings_layout::ROW_GAP);
}

void DevicePicker::update()
{
	if (describe_state() != shown_state)
		rebuild();
}

juce::String DevicePicker::describe_state() const
{
	const auto setup = device_manager.getAudioDeviceSetup();
	const auto* type = device_manager.getCurrentDeviceTypeObject();
	auto* device = device_manager.getCurrentAudioDevice();

	juce::StringArray parts{device_manager.getCurrentAudioDeviceType(), setup.outputDeviceName, setup.inputDeviceName, juce::String(context.settings.jack_buffer_size)};

	if (device != nullptr)
		parts.add(juce::String(device->getCurrentSampleRate()) + "/" + juce::String(device->getCurrentBufferSizeSamples()));

	if (type != nullptr) {
		parts.addArray(type->getDeviceNames(false));
		parts.addArray(type->getDeviceNames(true));
	}

	return parts.joinIntoString("|");
}

bool DevicePicker::is_jack() const
{
	return device_manager.getCurrentAudioDeviceType() == DeviceSession::JACK;
}

void DevicePicker::rebuild()
{
	list_drivers();
	list_devices();
	list_sample_rates();
	list_buffer_sizes();
	shown_state = describe_state();
}

void DevicePicker::list_drivers()
{
	juce::StringArray names;

	for (const auto* type : device_manager.getAvailableDeviceTypes())
		names.add(type->getTypeName());

	fill(driver.box, names, names.indexOf(device_manager.getCurrentAudioDeviceType()));
}

void DevicePicker::list_devices()
{
	const auto* type = device_manager.getCurrentDeviceTypeObject();
	const auto setup = device_manager.getAudioDeviceSetup();
	const auto outputs = type != nullptr ? type->getDeviceNames(false) : juce::StringArray{};
	const auto inputs = type != nullptr ? type->getDeviceNames(true) : juce::StringArray{};

	fill(output.box, outputs, outputs.indexOf(setup.outputDeviceName));
	fill(input.box, inputs, inputs.indexOf(setup.inputDeviceName));
}

void DevicePicker::list_sample_rates()
{
	auto* device = device_manager.getCurrentAudioDevice();
	sample_rates.clear();
	juce::StringArray texts;

	if (device != nullptr)
		for (const auto rate : device->getAvailableSampleRates()) {
			sample_rates.push_back(rate);
			texts.add(juce::String(rate, 0) + " Hz");
		}

	fill(sample_rate.box, texts, device != nullptr ? index_of(sample_rates, device->getCurrentSampleRate()) : NOTHING_SELECTED);
}

void DevicePicker::list_buffer_sizes()
{
	auto* device = device_manager.getCurrentAudioDevice();
	const auto rate = device != nullptr ? device->getCurrentSampleRate() : 0.0;
	buffer_sizes.clear();

	if (is_jack())
		buffer_sizes = JACK_BUFFER_SIZES;
	else if (device != nullptr)
		for (const auto size : device->getAvailableBufferSizes())
			if (is_power_of_two(size) && size <= LARGEST_BUFFER_SIZE)
				buffer_sizes.push_back(size);

	juce::StringArray texts;

	for (const auto size : buffer_sizes)
		texts.add(describe_buffer(size, rate));

	const auto current = is_jack() ? context.settings.jack_buffer_size : (device != nullptr ? device->getCurrentBufferSizeSamples() : 0);
	fill(buffer_size.box, texts, index_of(buffer_sizes, current));
}

void DevicePicker::choose_driver()
{
	device_manager.setCurrentAudioDeviceType(driver.box.getText(), true);
}

void DevicePicker::choose_output()
{
	auto setup = device_manager.getAudioDeviceSetup();
	setup.outputDeviceName = output.box.getText();
	device_manager.setAudioDeviceSetup(setup, true);
}

void DevicePicker::choose_input()
{
	auto setup = device_manager.getAudioDeviceSetup();
	setup.inputDeviceName = input.box.getText();
	device_manager.setAudioDeviceSetup(setup, true);
}

void DevicePicker::choose_sample_rate()
{
	auto setup = device_manager.getAudioDeviceSetup();
	setup.sampleRate = sample_rates[(size_t) sample_rate.box.getSelectedItemIndex()];
	device_manager.setAudioDeviceSetup(setup, true);
}

void DevicePicker::choose_buffer_size()
{
	const auto size = buffer_sizes[(size_t) buffer_size.box.getSelectedItemIndex()];

	if (is_jack()) {
		context.settings.jack_buffer_size = size;
		context.settings.save();
		return;
	}

	auto setup = device_manager.getAudioDeviceSetup();
	setup.bufferSize = size;
	device_manager.setAudioDeviceSetup(setup, true);
}

void DevicePicker::rescan()
{
	context.devices.rescan_devices();
	rebuild();
}

void DevicePicker::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds();

	for (auto* row : rows) {
		auto line = area.removeFromTop(ROW_HEIGHT);
		row->label.setBounds(line.removeFromLeft(LABEL_WIDTH));
		row->box.setBounds(line.removeFromLeft(FIELD_WIDTH));
		area.removeFromTop(ROW_GAP);
	}

	rescan_button.setBounds(area.removeFromTop(ROW_HEIGHT).withTrimmedLeft(LABEL_WIDTH).withWidth(BUTTON_WIDTH));
}
