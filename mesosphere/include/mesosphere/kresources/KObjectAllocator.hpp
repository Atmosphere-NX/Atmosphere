#pragma once
#include <boost/intrusive/set.hpp>
#include <mesosphere/core/util.hpp>
#include <mesosphere/kresources/KSlabHeap.hpp>
#include <mesosphere/threading/KMutex.hpp>

namespace mesosphere
{

template<typename T>
class KObjectAllocator {
    private:
    struct Comparator {
        constexpr bool operator()(const T &lhs, const T &rhs) const
        {
            return lhs.GetComparisonKey() < rhs.GetComparisonKey();
        }
    };


    struct ComparatorEqual {
        constexpr u64 operator()(const T &val) const
        {
            return val.GetComparisonKey();
        }
    };

    public:
    struct HookTag;

    using AllocatedSetHookType = boost::intrusive::set_base_hook<
        boost::intrusive::tag<HookTag>,
        boost::intrusive::link_mode<boost::intrusive::normal_link>
    >;
    using AllocatedSetType = typename
    boost::intrusive::make_set<
        T,
        boost::intrusive::base_hook<AllocatedSetHookType>,
        boost::intrusive::compare<Comparator>
    >::type;

    using pointer = T *;
    using const_pointer = const T *;
    using void_pointer = void *;
    using const_void_ptr = const void *;
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    AllocatedSetType &GetAllocatedSet()
    {
        return allocatedSet;
    }

    KSlabHeap<T> &GetSlabHeap()
    {
        return slabHeap;
    }

    void RegisterObject(T &obj) noexcept
    {
        std::scoped_lock guard{mutex};
        allocatedSet.insert(obj);
    }

    void UnregisterObject(T &obj) noexcept
    {
        std::scoped_lock guard{mutex};
        allocatedSet.erase(obj);
    }

    SharedPtr<T> FindObject(u64 comparisonKey) noexcept
    {
        std::scoped_lock guard{mutex};
        auto it = allocatedSet.find(comparisonKey, ComparatorEqual{});
        return it != allocatedSet.end() ? &*it : nullptr;
    }

    private:
    AllocatedSetType allocatedSet{};
    KSlabHeap<T> slabHeap{};
    mutable KMutex mutex{};
};

}
