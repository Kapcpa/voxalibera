#pragma once

#include "core/app_context.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <vector>

class DevicePicker : public juce::Component
{
public:
	explicit DevicePicker(AppContext& context);

	void update();
	void resized() override;

	static int preferred_height();

private:
	struct Row
	{
		juce::Label label;
		juce::ComboBox box;
	};

	juce::String describe_state() const;
	bool is_jack() const;
	void rebuild();
	void list_drivers();
	void list_devices();
	void list_sample_rates();
	void list_buffer_sizes();
	void choose_driver();
	void choose_output();
	void choose_input();
	void choose_sample_rate();
	void choose_buffer_size();
	void rescan();

	AppContext& context;
	juce::AudioDeviceManager& device_manager;
	Row driver;
	Row output;
	Row input;
	Row sample_rate;
	Row buffer_size;
	std::array<Row*, 5> rows{&driver, &output, &input, &sample_rate, &buffer_size};
	juce::TextButton rescan_button{"Rescan"};
	std::vector<double> sample_rates;
	std::vector<int> buffer_sizes;
	juce::String shown_state;
};
