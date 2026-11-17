#include "ui/export_dialog.h"

#include "ui/settings/settings_parts.h"
#include "ui/theme.h"

#include <cmath>
#include <initializer_list>

namespace
{
	constexpr int WIDTH = 560;
	constexpr int HEIGHT = 232;
	constexpr int WHOLE_SONG = 1;
	constexpr int LOOP_RANGE = 2;
	constexpr int SECONDS_PER_MINUTE = 60;
	constexpr auto WAV_PATTERN = "*.wav";
	constexpr auto WAV_EXTENSION = "wav";
	constexpr auto UNTITLED_FILE = "Untitled.wav";

	juce::String describe_length(std::int64_t samples, double rate)
	{
		const auto seconds = (int) std::ceil((double) samples / rate);
		return juce::String(seconds / SECONDS_PER_MINUTE) + ":" + juce::String(seconds % SECONDS_PER_MINUTE).paddedLeft('0', 2);
	}
}

ExportDialog::ExportDialog(AppContext& context) : context(context)
{
	format.addItem("WAV 16-bit (dithered)", export_format::DITHERED_16_BIT);
	format.addItem("WAV 24-bit", export_format::INTEGER_24_BIT);
	format.addItem("WAV 32-bit float", export_format::FLOAT_32_BIT);
	format.setSelectedId(export_format::INTEGER_24_BIT, juce::dontSendNotification);
	range.addItem("Whole song", WHOLE_SONG);
	range.addItem("Loop range", LOOP_RANGE);
	range.setSelectedId(WHOLE_SONG, juce::dontSendNotification);
	range.onChange = [this] { details.setText(describe_mix(), juce::dontSendNotification); };

	file_label.setColour(juce::Label::textColourId, theme::TEXT);
	file_label.setMinimumHorizontalScale(1.0f);
	details.setColour(juce::Label::textColourId, theme::DIM_TEXT);
	progress_bar.setColour(juce::ProgressBar::foregroundColourId, theme::ACCENT);
	progress_bar.setColour(juce::ProgressBar::backgroundColourId, theme::WIDGET_BACKGROUND);

	choose_button.onClick = [this] { choose_file(); };
	cancel_button.onClick = [this] { cancel(); };
	export_button.onClick = [this] { start(); };

	for (auto* child : std::initializer_list<juce::Component*>{&file_title, &file_label, &choose_button, &format_title, &format, &range_title, &range, &details, &cancel_button, &export_button})
		addAndMakeVisible(child);

	addChildComponent(progress_bar);
	setSize(WIDTH, HEIGHT);
}

void ExportDialog::prepare()
{
	if (file == juce::File() || file_project != context.session.project_file())
		use_file(default_file());

	file_project = context.session.project_file();
	refresh_range();
	show_running(false);
}

void ExportDialog::refresh_range()
{
	range.setItemEnabled(LOOP_RANGE, has_loop_range());

	if (!has_loop_range())
		range.setSelectedId(WHOLE_SONG, juce::dontSendNotification);

	details.setText(describe_mix(), juce::dontSendNotification);
}

bool ExportDialog::has_loop_range() const
{
	return context.project.loop.end > context.project.loop.start;
}

bool ExportDialog::exports_loop_range() const
{
	return has_loop_range() && range.getSelectedId() == LOOP_RANGE;
}

std::int64_t ExportDialog::export_length(double rate) const
{
	if (!exports_loop_range())
		return project_end(context.project);

	const auto& loop = context.project.loop;
	return ticks_to_samples(loop.end, context.project.tempo, rate) - ticks_to_samples(loop.start, context.project.tempo, rate);
}

juce::File ExportDialog::default_file() const
{
	if (context.session.is_saved())
		return context.session.project_file().withFileExtension(WAV_EXTENSION);

	return juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile(UNTITLED_FILE);
}

void ExportDialog::use_file(const juce::File& chosen)
{
	file = chosen.withFileExtension(WAV_EXTENSION);
	file_label.setText(file.getFullPathName(), juce::dontSendNotification);
	details.setText(describe_mix(), juce::dontSendNotification);
}

juce::String ExportDialog::describe_mix() const
{
	const auto rate = context.export_rate();

	if (rate <= 0.0)
		return {};

	const auto replaces = file.existsAsFile() ? ", replaces the existing file" : "";
	return juce::String(rate, 0) + " Hz stereo, " + describe_length(export_length(rate), rate) + " plus effect tails" + replaces;
}

void ExportDialog::choose_file()
{
	chooser = std::make_unique<juce::FileChooser>("Export Mix", file, WAV_PATTERN);

	constexpr auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting;

	chooser->launchAsync(flags, [this](const juce::FileChooser& finished) {
		if (const auto chosen = finished.getResult(); chosen != juce::File())
			use_file(chosen);
	});
}

void ExportDialog::start()
{
	const auto problem = context.start_export(file, format.getSelectedId(), exports_loop_range());

	if (problem.isNotEmpty()) {
		on_finished({ExportResult::Outcome::failed, file, 0.0f, false, problem});
		return;
	}

	show_running(true);
}

void ExportDialog::cancel()
{
	if (context.exporter.is_running()) {
		context.exporter.cancel();
		return;
	}

	on_closed();
}

void ExportDialog::update()
{
	progress = context.exporter.progress();
	const auto result = context.exporter.take_result();

	if (!result)
		return;

	show_running(false);

	if (result->outcome != ExportResult::Outcome::cancelled && isShowing())
		on_finished(*result);
}

void ExportDialog::show_running(bool running)
{
	progress_bar.setVisible(running);
	details.setVisible(!running);
	export_button.setEnabled(!running);
	choose_button.setEnabled(!running);
	format.setEnabled(!running);
	range.setEnabled(!running);
}

void ExportDialog::resized()
{
	using namespace settings_layout;
	auto area = getLocalBounds().reduced(PAGE_PADDING);

	auto file_row = area.removeFromTop(ROW_HEIGHT);
	file_title.setBounds(file_row.removeFromLeft(LABEL_WIDTH));
	choose_button.setBounds(file_row.removeFromRight(BUTTON_WIDTH));
	file_row.removeFromRight(ROW_GAP);
	file_label.setBounds(file_row);
	area.removeFromTop(ROW_GAP);

	auto format_row = area.removeFromTop(ROW_HEIGHT);
	format_title.setBounds(format_row.removeFromLeft(LABEL_WIDTH));
	format.setBounds(format_row);
	area.removeFromTop(ROW_GAP);

	auto range_row = area.removeFromTop(ROW_HEIGHT);
	range_title.setBounds(range_row.removeFromLeft(LABEL_WIDTH));
	range.setBounds(range_row);
	area.removeFromTop(ROW_GAP);

	auto status_row = area.removeFromTop(ROW_HEIGHT);
	status_row.removeFromLeft(LABEL_WIDTH);
	details.setBounds(status_row);
	progress_bar.setBounds(status_row);

	auto buttons = area.removeFromBottom(ROW_HEIGHT);
	export_button.setBounds(buttons.removeFromRight(BUTTON_WIDTH));
	buttons.removeFromRight(ROW_GAP);
	cancel_button.setBounds(buttons.removeFromRight(BUTTON_WIDTH));
}
