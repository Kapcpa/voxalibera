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

struct BiquadCoefficients
{
	float b0 = 1.0f;
	float b1 = 0.0f;
	float b2 = 0.0f;
	float a1 = 0.0f;
	float a2 = 0.0f;

	static BiquadCoefficients lowpass(double sample_rate, float frequency, float q);
	static BiquadCoefficients highpass(double sample_rate, float frequency, float q);
	static BiquadCoefficients peak(double sample_rate, float frequency, float q, float gain_db);
	static BiquadCoefficients low_shelf(double sample_rate, float frequency, float gain_db);
	static BiquadCoefficients high_shelf(double sample_rate, float frequency, float gain_db);
};

class Biquad
{
public:
	void set(const BiquadCoefficients& next) { coefficients = next; }
	void reset() { first = second = 0.0f; }
	float process(float input);

private:
	BiquadCoefficients coefficients;
	float first = 0.0f;
	float second = 0.0f;
};

float audio_taper(float position);
