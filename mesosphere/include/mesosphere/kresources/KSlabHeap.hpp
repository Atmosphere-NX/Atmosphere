#pragma once

#include <mesosphere/kresources/KSlabStack.hpp>

namespace mesosphere
{

template<typename T>
class KSlabHeap {
    public:
    using pointer = T *;
    using const_pointer = const T *;
    using void_pointer = void *;
    using const_void_ptr = const void *;
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    private:
    KSlabStack<T> stack{};
    size_t capacity = 0;
    T *bufferStart = nullptr;

    public:
    T *allocate() noexcept
    {
        return stack.pop();
    }

    void deallocate(T *elem) noexcept
    {
        kassert(elem >= bufferStart && elem < bufferStart + capacity);
        stack.push(elem);
    }

    constexpr size_t size() const
    {
        return capacity;
    }

    KSlabHeap() noexcept = default;

    void initialize(void *buffer, size_t capacity)
    {
        this->capacity = capacity;
        this->bufferStart = (T *)buffer;
        stack.initialize(buffer, capacity);
    }

    KSlabHeap(void *buffer, size_t capacity) noexcept : stack(buffer, capacity), capacity(capacity), bufferStart((T *)buffer)
    {
    }
};

}
