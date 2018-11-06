#pragma once

#include <mesosphere/kresources/KObjectAllocator.hpp>

namespace mesosphere
{

template<typename Derived>
class ISetAllocated : public KObjectAllocator<Derived>::AllocatedSetHookType
{
    public:
    static void InitializeAllocator(void *buffer, size_t capacity) noexcept
    {
        allocator.GetSlabHeap().initialize(buffer, capacity);
    }

    void *operator new(size_t sz) noexcept
    {
        kassert(sz == sizeof(Derived));
        return allocator.GetSlabHeap().allocate();
    }

    void operator delete(void *ptr) noexcept
    {
        allocator.GetSlabHeap().deallocate((Derived *)ptr);
    }

    void AddToAllocatedSet() noexcept
    {
        Derived *d = (Derived *)this;
        allocator.RegisterObject(*d);
        isRegisteredToAllocator = true;
    }

    protected:
    void RemoveFromAllocatedSet() noexcept
    {
        Derived *d = (Derived *)this;
        allocator.UnregisterObject(*d);
    }

    virtual ~ISetAllocated()
    {
        if (isRegisteredToAllocator) {
            RemoveFromAllocatedSet();
            isRegisteredToAllocator = false;
        }
    }

    private:
    bool isRegisteredToAllocator = false;

    protected:
    static KObjectAllocator<Derived> allocator;
};

template<typename Derived>
KObjectAllocator<Derived> ISetAllocated<Derived>::allocator{};

}