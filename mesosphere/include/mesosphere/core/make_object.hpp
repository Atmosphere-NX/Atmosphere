#pragma once

#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/processes/KProcess.hpp>
#include <mesosphere/kresources/KResourceLimit.hpp>
#include <mesosphere/interfaces/IClientServerParent.hpp>
#include <type_traits>

namespace mesosphere
{

template<typename T, typename ...Args>
auto MakeObjectRaw(Args&& ...args)
{
    Result res = ResultSuccess;
    KCoreContext &cctx = KCoreContext::GetCurrentInstance();
    auto reslimit = cctx.GetCurrentProcess()->GetResourceLimit();
    bool doReslimitCleanup = false;
    T *obj = nullptr;
    if constexpr (std::is_base_of_v<ILimitedResource<T>, T>) {
        if (reslimit != nullptr) {
            if (reslimit->Reserve(KResourceLimit::GetCategoryOf<T>(), 1, maxResourceAcqWaitTime)) {
                doReslimitCleanup = true;
            } else {
                return ResultKernelOutOfResource();
            }
        }
    }

    obj = new T;
    if (obj == nullptr) {
        res = ResultKernelResourceExhausted();
        goto cleanup;
    }

    res = T::Initialize(std::forward<Args>(args)...);
    if (res.IsSuccess()) {
        doReslimitCleanup = false;
        if constexpr (std::is_base_of_v<ISetAllocated<T>, T>) {
            obj->AddToAllocatedSet();
        }
    }
cleanup:
    if (doReslimitCleanup) {
        reslimit->Release(KResourceLimit::GetCategoryOf<T>(), 1, 1);
    }

    return std::tuple{res, obj};
}

template<typename T, typename = std::enable_if_t<!std::is_base_of_v<IClientServerParentTag, T>>, typename ...Args>
auto MakeObject(Args&& ...args)
{
    auto [res, obj] = MakeObjectRaw(std::forward<Args>(args)...);
    return std::tuple{res, SharedPtr{obj}};
}

template<typename T, typename = std::enable_if_t<std::is_base_of_v<IClientServerParentTag, T>>, typename ...Args>
auto MakeObject(Args&& ...args)
{
    auto [res, obj] = MakeObjectRaw(std::forward<Args>(args)...);
    return res.IsSuccess() ? std::tuple{res, SharedPtr{&obj.GetServer()}, SharedPtr{&obj.GetClient()}} : std::tuple{res, nullptr, nullptr};
}

template<typename T, typename = std::enable_if_t<!std::is_base_of_v<IClientServerParentTag, T>>, typename ...Args>
auto MakeObjectWithHandle(Args&& ...args)
{
    KCoreContext &cctx = KCoreContext::GetCurrentInstance();
    KProcess *currentProcess = cctx.GetCurrentProcess();
    KHandleTable &tbl = currentProcess->GetHandleTable();

    auto [res, obj] = MakeObjectRaw(std::forward<Args>(args)...);
    if (res.IsFailure()) {
        return std::tuple{res, Handle{}};
    }

    return tbl.Generate(obj);
}

template<typename T, typename = std::enable_if_t<std::is_base_of_v<IClientServerParentTag, T>>, typename ...Args>
auto MakeObjectWithHandle(Args&& ...args)
{
    KCoreContext &cctx = KCoreContext::GetCurrentInstance();
    KProcess *currentProcess = cctx.GetCurrentProcess();
    KHandleTable &tbl = currentProcess->GetHandleTable();

    auto [res, obj] = MakeObjectRaw(std::forward<Args>(args)...);
    if (res.IsFailure()) {
        return std::tuple{res, Handle{}, Handle{}};
    }

    auto [res2, serverHandle] = tbl.Generate(&obj.GetServer());
    if (res2.IsSuccess()) {
        auto [res3, clientHandle] = tbl.Generate(&obj.GetClient());
        if (res3.IsSuccess()) {
            return std::tuple{res3, serverHandle, clientHandle};
        } else {
            tbl.Close(serverHandle);
            return std::tuple{res3, Handle{}, Handle{}};
        }
    } else {
        return std::tuple{res2, Handle{}, Handle{}};
    }
}

}