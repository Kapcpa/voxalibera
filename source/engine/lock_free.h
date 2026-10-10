#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

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

template <typename Snapshot>
class SnapshotExchange
{
public:
	~SnapshotExchange()
	{
		delete current.load();
	}

	void publish(std::unique_ptr<Snapshot> snapshot)
	{
		auto* replaced = current.exchange(snapshot.release(), std::memory_order_acq_rel);

		if (replaced != nullptr)
			retired.push_back({std::unique_ptr<Snapshot>(replaced), finished_blocks.load(std::memory_order_acquire)});
	}

	void release_retired(bool audio_running)
	{
		const auto finished = finished_blocks.load(std::memory_order_acquire);
		std::erase_if(retired, [&](const Retired& entry) { return !audio_running || finished > entry.retired_after_block; });
	}

	Snapshot* begin_block() const
	{
		return current.load(std::memory_order_acquire);
	}

	void end_block()
	{
		finished_blocks.fetch_add(1, std::memory_order_release);
	}

private:
	struct Retired
	{
		std::unique_ptr<Snapshot> snapshot;
		std::uint64_t retired_after_block = 0;
	};

	std::atomic<Snapshot*> current{nullptr};
	std::atomic<std::uint64_t> finished_blocks{0};
	std::vector<Retired> retired;
};
