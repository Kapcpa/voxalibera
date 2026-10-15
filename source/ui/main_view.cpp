#include "ui/main_view.h"

#include "ui/theme.h"

namespace
{
	constexpr int WIDTH = 1360;
	constexpr int HEIGHT = 880;
	constexpr int TRANSPORT_HEIGHT = 48;
	constexpr int MIXER_HEIGHT = 260;
	constexpr int PLUGIN_CHAIN_HEIGHT = 160;
	constexpr int GAP = 2;
}

MainView::MainView(AppContext& context) : transport_bar(context), track_area(context), plugin_chain(context), mixer(context)
{
	addAndMakeVisible(transport_bar);
	addAndMakeVisible(track_area);
	addAndMakeVisible(mixer);
	addAndMakeVisible(plugin_chain);

	setSize(WIDTH, HEIGHT);
}

void MainView::update()
{
	transport_bar.update();
	track_area.update();
	mixer.update();
	plugin_chain.update();
}

void MainView::set_mixer_visible(bool visible)
{
	mixer.setVisible(visible);
	resized();
}

void MainView::set_plugin_chain_visible(bool visible)
{
	plugin_chain.setVisible(visible);
	resized();
}

void MainView::paint(juce::Graphics& graphics)
{
	graphics.fillAll(theme::WINDOW_BACKGROUND);
}

void MainView::resized()
{
	auto area = getLocalBounds();

	transport_bar.setBounds(area.removeFromTop(TRANSPORT_HEIGHT));
	area.removeFromTop(GAP);

	if (mixer.isVisible()) {
		mixer.setBounds(area.removeFromBottom(MIXER_HEIGHT));
		area.removeFromBottom(GAP);
	}

	if (plugin_chain.isVisible()) {
		plugin_chain.setBounds(area.removeFromBottom(PLUGIN_CHAIN_HEIGHT));
		area.removeFromBottom(GAP);
	}

	track_area.setBounds(area);
}
