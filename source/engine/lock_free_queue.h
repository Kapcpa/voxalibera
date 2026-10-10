#pragma once

#include <juce_core/juce_core.h>

#include <array>

template <typename Item, size_t CAPACITY>
class LockFreeQueue
{
public:
	bool push(const Item& item)
	{
		const auto scope = fifo.write(1);

		if (scope.blockSize1 == 0)
			return false;

		items[(size_t) scope.startIndex1] = item;
		return true;
	}

	bool pop(Item& item)
	{
		const auto scope = fifo.read(1);

		if (scope.blockSize1 == 0)
			return false;

		item = items[(size_t) scope.startIndex1];
		return true;
	}

private:
	juce::AbstractFifo fifo{(int) CAPACITY};
	std::array<Item, CAPACITY> items{};
};
