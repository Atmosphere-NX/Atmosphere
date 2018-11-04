#pragma once
// circular dep: #include "resource_limit.h"

#include <mesosphere/core/KAutoObject.hpp>
#include <tuple>

namespace mesosphere
{

namespace detail
{

void ReleaseResource(const SharedPtr<KProcess> &owner, KAutoObject::TypeId typeId, size_t count, size_t realCount);
void ReleaseResource(const SharedPtr<KResourceLimit> &reslimit, KAutoObject::TypeId typeId, size_t count, size_t realCount);

}

template<typename Derived>
class ILimitedResource {
    public:

    const SharedPtr<KProcess>& GetResourceOwner() const { return resourceOwner; }
    void SetResourceOwner(SharedPtr<KProcess> owner)
    {
        resourceOwner = std::move(owner);
        isLimitedResourceActive = true;
    }

    virtual std::tuple<size_t, size_t> GetResourceCount()
    {
        return {1, 1}; // current, real
    }

    ~ILimitedResource()
    {
        if (isLimitedResourceActive) {
            auto [cur, real] = GetResourceCount();
            detail::ReleaseResource(resourceOwner, Derived::typeId, cur, real);
        }
    }

    private:
    SharedPtr<KProcess> resourceOwner{};
    bool isLimitedResourceActive = false;
};

}
