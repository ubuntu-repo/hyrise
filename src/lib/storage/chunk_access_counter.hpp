#pragma once

#include <atomic> // NEEDEDINCLUDE
#include <boost/circular_buffer.hpp> // NEEDEDINCLUDE

#include "types.hpp" // NEEDEDINCLUDE
#include "utils/polymorphic_allocator.hpp" // NEEDEDINCLUDE

namespace opossum {

/**
 * Data structure for storing chunk access times
 *
 * The chunk access times are tracked using ProxyChunk objects
 * that measure the cycles they were in scope using the RDTSC instructions.
 * The access times are added to a counter. The ChunkMetricCollection tasks
 * is regularly scheduled by the NUMAPlacementManager. This tasks takes a snapshot
 * of the current counter values and places them in a history. The history is
 * stored in a ring buffer, so that only a limited number of history items are
 * preserved.
 */
struct ChunkAccessCounter {
  friend class Chunk;

 public:
  explicit ChunkAccessCounter(const PolymorphicAllocator<uint64_t>& alloc) : _history(_capacity, alloc) {}

  void increment() { _counter++; }
  void increment(uint64_t value) { _counter.fetch_add(value); }

  // Takes a snapshot of the current counter and adds it to the history
  void process() { _history.push_back(_counter); }

  // Returns the access time of the chunk during the specified number of
  // recent history sample iterations.
  uint64_t history_sample(size_t lookback) const;

  uint64_t counter() const { return _counter; }

 private:
  const size_t _capacity = 100;
  std::atomic<std::uint64_t> _counter{0};
  boost::circular_buffer<uint64_t, PolymorphicAllocator<uint64_t>> _history;
};

}  // namespace opossum
