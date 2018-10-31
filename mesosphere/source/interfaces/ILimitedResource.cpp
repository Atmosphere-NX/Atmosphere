#include <mesosphere/interfaces/ILimitedResource.hpp>
#include <mesosphere/processes/KProcess.hpp>
#include <mesosphere/kresources/KResourceLimit.hpp>

namespace mesosphere::detail
{

void ReleaseResource(const SharedPtr<KResourceLimit> &reslimit, KAutoObject::TypeId typeId, size_t count, size_t realCount)
{
    if (reslimit != nullptr) {
        reslimit->Release(KResourceLimit::GetCategory(typeId), count, realCount);
    } else {
        KResourceLimit::GetDefaultInstance().Release(KResourceLimit::GetCategory(typeId), count, realCount);
    }
}

void ReleaseResource(const SharedPtr<KProcess> &owner, KAutoObject::TypeId typeId, size_t count, size_t realCount)
{
    if (owner != nullptr) {
        return ReleaseResource(owner->GetResourceLimit(), typeId, count, realCount);
    }  else {
        KResourceLimit::GetDefaultInstance().Release(KResourceLimit::GetCategory(typeId), count, realCount);
    }
}

}
