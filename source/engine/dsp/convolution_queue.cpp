#include "engine/dsp/convolution_queue.h"

juce::dsp::ConvolutionMessageQueue& convolution_queue()
{
	static juce::dsp::ConvolutionMessageQueue queue;
	return queue;
}
