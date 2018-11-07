#pragma once

#include <mesosphere/core/util.hpp>
#include <mesosphere/core/Handle.hpp>
#include <mesosphere/core/Result.hpp>
#include <mesosphere/core/KAutoObject.hpp>
#include <mesosphere/interrupts/KInterruptSpinLock.hpp>
#include <array>
#include <tuple>

namespace mesosphere
{

class KThread;
class KProcess;

class KHandleTable final {
    public:

    static constexpr size_t capacityLimit = 1024;
    static constexpr Handle selfThreadAlias{0, -1, true};
    static constexpr Handle selfProcessAlias{1, -1, true};

    template<typename T>
    SharedPtr<T> Get(Handle handle, bool allowAlias = true) const
    {
        if constexpr (std::is_same_v<T, KAutoObject>) {
            (void)allowAlias;
            return GetAutoObject(handle);
        } else if constexpr (std::is_same_v<T, KThread>) {
            return GetThread(handle, allowAlias);
        } else if constexpr (std::is_same_v<T, KProcess>) {
            return GetProcess(handle, allowAlias);
        } else {
            return DynamicObjectCast<T>(GetAutoObject(handle));
        }
    }

    std::tuple<Result, Handle> Generate(SharedPtr<KAutoObject> obj);

    /// For deferred-init
    Result Set(SharedPtr<KAutoObject> obj, Handle handle);

    bool Close(Handle handle);
    void Destroy();

    constexpr size_t GetNumActive() const { return numActive; }
    constexpr size_t GetSize() const { return size; }
    constexpr size_t GetCapacity() const { return capacity; }

    Result Initialize(size_t capacity); // TODO: implement!

    ~KHandleTable();

    private:

    bool IsValid(Handle handle) const;
    SharedPtr<KAutoObject> GetAutoObject(Handle handle) const;
    SharedPtr<KThread> GetThread(Handle handle, bool allowAlias = true) const;
    SharedPtr<KProcess> GetProcess(Handle handle, bool allowAlias = true) const;

    struct Entry {
        SharedPtr<KAutoObject> object{};
        s16 id = 0;
    };

    std::array<Entry, capacityLimit> entries{};

    // Here the official kernel uses pointer, Yuzu and ourselves are repurposing a field in Entry instead.
    s16 firstFreeIndex = 0;
    s16 idCounter = 1;

    u16 numActive = 0, size = 0, capacity = 0;

    mutable KInterruptSpinLock<false> spinlock;
};

}
