#pragma once

class OnePole
{
public:
	void set_cutoff(float frequency, double sample_rate);
	void reset() { state = 0.0f; }

	float lowpass(float input);
	float highpass(float input) { return input - lowpass(input); }

private:
	float gain = 0.0f;
	float state = 0.0f;
};

enum class FilterShape
{
	bell,
	low_shelf,
	high_shelf,
	lowpass,
	highpass
};

struct SvfCoefficients
{
	float g = 0.0f;
	float k = 1.0f;
	float a1 = 1.0f;
	float a2 = 0.0f;
	float a3 = 0.0f;
	float input_mix = 1.0f;
	float band_mix = 0.0f;
	float low_mix = 0.0f;

	static SvfCoefficients make(FilterShape shape, double sample_rate, float frequency, float q, float gain_db = 0.0f);
	float magnitude_db(double sample_rate, float frequency) const;
};

class Svf
{
public:
	void set(const SvfCoefficients& next) { coefficients = next; }
	void reset() { band_state = low_state = 0.0f; }
	float process(float input);

private:
	SvfCoefficients coefficients;
	float band_state = 0.0f;
	float low_state = 0.0f;
};

float butterworth_q(int order, int section);
float octave_bandwidth_q(float octaves);

float audio_taper(float position);
