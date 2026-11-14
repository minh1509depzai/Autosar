#ifndef DOIP_UTIL_RING_BUFFER_HPP
#define DOIP_UTIL_RING_BUFFER_HPP

#include <cstddef>

namespace as {
namespace doip {
namespace util {
/*
 * Fixed-capacity FIFO ring buffer with static storage (no heap allocation).
 * Type must be default-constructible and copy-assignable.
 * Not thread-safe: external synchronisation is required if the producer and
 * the consumer run in different contexts.
 */
template <typename Type, std::size_t Size>
class RingBuffer
{
    static_assert(Size > 0, "RingBuffer size must be greater than zero");

public:
    typedef Type value_type;

    static std::size_t capacity() { return Size; }

    std::size_t size() const { return count_; }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == Size; }

    // Appends an element; returns false (buffer unchanged) when full.
    bool push(const Type& value)
    {
        if (full()) {
            return false;
        }
        data_[advance(head_, count_)] = value;
        ++count_;
        return true;
    }

    // Appends an element; when full the oldest element is overwritten.
    void push_overwrite(const Type& value)
    {
        if (full()) {
            data_[head_] = value;
            head_ = advance(head_, 1);
            return;
        }
        push(value);
    }

    // Removes the oldest element into 'value'; returns false when empty.
    bool pop(Type& value)
    {
        if (empty()) {
            return false;
        }
        value = data_[head_];
        head_ = advance(head_, 1);
        --count_;
        return true;
    }

    // Reads the oldest element without removing it; false when empty.
    bool peek(Type& value) const
    {
        if (empty()) {
            return false;
        }
        value = data_[head_];
        return true;
    }

    // Reads element 'index' counted from the oldest; false when out of range.
    bool at(std::size_t index, Type& value) const
    {
        if (index >= count_) {
            return false;
        }
        value = data_[advance(head_, index)];
        return true;
    }

    void clear()
    {
        head_ = 0;
        count_ = 0;
    }

private:
    static std::size_t advance(std::size_t position, std::size_t offset)
    {
        return (position + offset) % Size;
    }

    Type data_[Size];
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

} // namespace util
} // namespace doip
} // namespace as

#endif
