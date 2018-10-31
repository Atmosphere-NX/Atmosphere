#pragma once

#include <mesosphere/kresources/KSlabHeap.hpp>

namespace mesosphere
{

template<typename Derived>
class ISlabAllocated
{
    public:
    static void InitializeSlabHeap(void *buffer, size_t capacity) noexcept
    {
        slabHeap.initialize(buffer, capacity);
    }

    void *operator new(size_t sz) noexcept
    {
        kassert(sz == sizeof(Derived));
        return slabHeap.allocate();
    }

    void operator delete(void *ptr) noexcept
    {
        slabHeap.deallocate((Derived *)ptr);
    }

    protected:
    static KSlabHeap<Derived> slabHeap;
};

template<typename Derived>
KSlabHeap<Derived> ISlabAllocated<Derived>::slabHeap{};

}
