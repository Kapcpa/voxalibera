#include "engine/passthrough.h"

#include <catch2/catch_test_macros.hpp>

#include <array>

namespace
{
	using Block = std::array<float, 4>;

	struct StereoBlock
	{
		Block left{};
		Block right{};
	};

	StereoBlock run(Passthrough& passthrough, const Block& left_input, const Block& right_input, int input_count)
	{
		StereoBlock output;
		const float* inputs[]{left_input.data(), right_input.data()};
		float* outputs[]{output.left.data(), output.right.data()};
		passthrough.process(inputs, input_count, outputs, 2, (int) Block{}.size());
		return output;
	}

	const Block LEFT_SIGNAL{0.1f, 0.2f, 0.3f, 0.4f};
	const Block RIGHT_SIGNAL{-0.5f, -0.6f, -0.7f, -0.8f};
	const Block SILENCE{};
}

TEST_CASE("left input reaches both outputs")
{
	Passthrough passthrough;
	passthrough.routing = InputRouting::left_to_both;

	const auto output = run(passthrough, LEFT_SIGNAL, RIGHT_SIGNAL, 2);

	CHECK(output.left == LEFT_SIGNAL);
	CHECK(output.right == LEFT_SIGNAL);
}

TEST_CASE("stereo keeps channels apart")
{
	Passthrough passthrough;
	passthrough.routing = InputRouting::stereo;

	const auto output = run(passthrough, LEFT_SIGNAL, RIGHT_SIGNAL, 2);

	CHECK(output.left == LEFT_SIGNAL);
	CHECK(output.right == RIGHT_SIGNAL);
}

TEST_CASE("missing right input gives silence instead of garbage")
{
	Passthrough passthrough;
	passthrough.routing = InputRouting::right_to_both;

	const auto output = run(passthrough, LEFT_SIGNAL, RIGHT_SIGNAL, 1);

	CHECK(output.left == SILENCE);
	CHECK(output.right == SILENCE);
}

TEST_CASE("disabled monitoring outputs silence")
{
	Passthrough passthrough;
	passthrough.monitoring = false;

	const auto output = run(passthrough, LEFT_SIGNAL, RIGHT_SIGNAL, 2);

	CHECK(output.left == SILENCE);
	CHECK(output.right == SILENCE);
}
