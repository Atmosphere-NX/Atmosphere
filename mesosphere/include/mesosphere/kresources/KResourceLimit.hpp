#pragma once

#include <mesosphere/threading/KConditionVariable.hpp>

namespace mesosphere
{

class KThread;
class KEvent;
class KTransferMemory;
class KSession;

class KResourceLimit final :
    public KAutoObject,
    public ISetAllocated<KResourceLimit>
{
    public:

    MESOSPHERE_AUTO_OBJECT_TRAITS(AutoObject, ResourceLimit);

    enum class Category : uint {
        Memory = 0,
        Threads,
        Events,
        TransferMemories,
        Sessions,

        Max,
    };

    static constexpr Category GetCategory(KAutoObject::TypeId typeId) {
        switch (typeId) {
            case KAutoObject::TypeId::Thread:           return Category::Threads;
            case KAutoObject::TypeId::Event:            return Category::Events;
            case KAutoObject::TypeId::TransferMemory:   return Category::TransferMemories;
            case KAutoObject::TypeId::Session:          return Category::Sessions;
            default:                                    return Category::Max;
        }
    }

    template<typename T> static constexpr Category categoryOf = GetCategory(T::typeId);

    static KResourceLimit &GetDefaultInstance() { return defaultInstance; }

    size_t GetCurrentValue(Category category) const;
    size_t GetLimitValue(Category category) const;
    size_t GetRemainingValue(Category category) const;

    bool SetLimitValue(Category category, size_t value);

    template<typename Rep, typename Period>
    bool Reserve(Category category, size_t count, const std::chrono::duration<Rep, Period>& timeout)
    {
        return ReserveDetail(category, count, KSystemClock::now() + timeout);
    }

    void Release(Category category, size_t count, size_t realCount);

    private:

    static KResourceLimit defaultInstance;
    bool ReserveDetail(Category category, size_t count, const KSystemClock::time_point &timeoutTime);

    // Signed in official kernel
    size_t limitValues[(size_t)Category::Max] = {};

    // Current value: real value + dangling resources about to be released
    size_t currentValues[(size_t)Category::Max] = {};
    size_t realValues[(size_t)Category::Max] = {};

    mutable KConditionVariable condvar{};
};

inline void intrusive_ptr_add_ref(KResourceLimit *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}

inline void intrusive_ptr_release(KResourceLimit *obj)
{
    intrusive_ptr_add_ref((KAutoObject *)obj);
}
}
