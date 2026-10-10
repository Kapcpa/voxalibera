#include "ui/theme.h"

namespace
{
	constexpr float GROOVE_THICKNESS = 4.0f;
	constexpr float GROOVE_RADIUS = 2.0f;
	constexpr float CAP_LENGTH = 10.0f;
	constexpr float CAP_WIDTH = 22.0f;
	constexpr float CAP_RADIUS = 2.0f;
	constexpr float FILL_ALPHA = 0.75f;
	constexpr float KNOB_RING_THICKNESS = 3.0f;
	constexpr float KNOB_MARGIN = 2.0f;
	constexpr float POINTER_THICKNESS = 2.0f;
	constexpr float POINTER_INNER = 0.35f;

	bool is_bipolar(const juce::Slider& slider)
	{
		return slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
	}

	juce::Path arc(juce::Point<float> centre, float radius, float from_angle, float to_angle)
	{
		juce::Path path;
		path.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, from_angle, to_angle, true);
		return path;
	}

	juce::LookAndFeel_V4::ColourScheme theme_colours()
	{
		return {theme::WINDOW_BACKGROUND, theme::WIDGET_BACKGROUND, theme::MENU_BACKGROUND, theme::PANEL_BORDER, theme::TEXT, theme::ACCENT, theme::ACCENT_TEXT, theme::ACCENT, theme::TEXT};
	}
}

ThemeLookAndFeel::ThemeLookAndFeel() : LookAndFeel_V4(theme_colours())
{
	setColour(juce::ResizableWindow::backgroundColourId, theme::WINDOW_BACKGROUND);
	setColour(juce::DocumentWindow::backgroundColourId, theme::WINDOW_BACKGROUND);
	setColour(juce::ComboBox::backgroundColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::ComboBox::outlineColourId, theme::PANEL_BORDER);
	setColour(juce::TextButton::buttonColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::ListBox::backgroundColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::ListBox::outlineColourId, theme::PANEL_BORDER);
	setColour(juce::Slider::trackColourId, theme::ACCENT);
	setColour(juce::Slider::thumbColourId, theme::TEXT);
	setColour(juce::Slider::backgroundColourId, theme::WIDGET_BACKGROUND);
	setColour(juce::Slider::textBoxOutlineColourId, theme::PANEL_BORDER);
	setColour(juce::ScrollBar::thumbColourId, theme::PANEL_BORDER);
	setColour(juce::PopupMenu::backgroundColourId, theme::MENU_BACKGROUND);
	setColour(juce::PopupMenu::highlightedBackgroundColourId, theme::ACCENT);
}

int ThemeLookAndFeel::getSliderThumbRadius(juce::Slider&)
{
	return SLIDER_END_MARGIN;
}

void ThemeLookAndFeel::drawLinearSlider(juce::Graphics& graphics, int x, int y, int width, int height, float slider_position, float minimum_position, float maximum_position, juce::Slider::SliderStyle style, juce::Slider& slider)
{
	const auto vertical = style == juce::Slider::LinearVertical;

	if (!vertical && style != juce::Slider::LinearHorizontal) {
		LookAndFeel_V4::drawLinearSlider(graphics, x, y, width, height, slider_position, minimum_position, maximum_position, style, slider);
		return;
	}

	const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
	const auto groove = vertical ? bounds.withSizeKeepingCentre(GROOVE_THICKNESS, bounds.getHeight()) : bounds.withSizeKeepingCentre(bounds.getWidth(), GROOVE_THICKNESS);
	const auto filled = vertical ? groove.withTop(slider_position) : groove.withRight(slider_position);
	const auto cap = vertical ? juce::Rectangle<float>(std::min(CAP_WIDTH, bounds.getWidth()), CAP_LENGTH).withCentre({bounds.getCentreX(), slider_position}) : juce::Rectangle<float>(CAP_LENGTH, std::min(CAP_WIDTH, bounds.getHeight())).withCentre({slider_position, bounds.getCentreY()});

	graphics.setColour(theme::GROOVE);
	graphics.fillRoundedRectangle(groove, GROOVE_RADIUS);
	graphics.setColour(theme::ACCENT.withAlpha(slider.isEnabled() ? FILL_ALPHA : FILL_ALPHA / 2.0f));
	graphics.fillRoundedRectangle(filled, GROOVE_RADIUS);

	graphics.setColour(theme::FADER_CAP);
	graphics.fillRoundedRectangle(cap, CAP_RADIUS);
	graphics.setColour(theme::FADER_CAP_LINE);

	if (vertical)
		graphics.drawHorizontalLine((int) cap.getCentreY(), cap.getX() + 2.0f, cap.getRight() - 2.0f);
	else
		graphics.drawVerticalLine((int) cap.getCentreX(), cap.getY() + 2.0f, cap.getBottom() - 2.0f);
}

void ThemeLookAndFeel::drawRotarySlider(juce::Graphics& graphics, int x, int y, int width, int height, float position, float start_angle, float end_angle, juce::Slider& slider)
{
	const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
	const auto radius = std::min(bounds.getWidth(), bounds.getHeight()) / 2.0f - KNOB_MARGIN - KNOB_RING_THICKNESS / 2.0f;
	const auto centre = bounds.getCentre();
	const auto angle = start_angle + position * (end_angle - start_angle);
	const auto origin = is_bipolar(slider) ? (start_angle + end_angle) / 2.0f : start_angle;
	const juce::PathStrokeType ring(KNOB_RING_THICKNESS, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

	graphics.setColour(theme::GROOVE);
	graphics.strokePath(arc(centre, radius, start_angle, end_angle), ring);
	graphics.setColour(theme::ACCENT);
	graphics.strokePath(arc(centre, radius, std::min(origin, angle), std::max(origin, angle)), ring);

	const auto pointer_end = centre.getPointOnCircumference(radius, angle);
	const auto pointer_start = centre.getPointOnCircumference(radius * POINTER_INNER, angle);
	graphics.setColour(theme::TEXT);
	graphics.drawLine({pointer_start, pointer_end}, POINTER_THICKNESS);
}
