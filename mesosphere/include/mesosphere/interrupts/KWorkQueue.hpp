#pragma once

#include <mesosphere/interfaces/IWork.hpp>
#include <mesosphere/kresources/KAutoObject.hpp>

namespace mesosphere
{

class KWorkQueue final {
    public:

    void AddWork(IWork &work);
    void Initialize();

    void HandleWorkQueue();

    KWorkQueue(const KWorkQueue &) = delete;
    KWorkQueue(KWorkQueue &&) = delete;
    KWorkQueue &operator=(const KWorkQueue &) = delete;
    KWorkQueue &operator=(KWorkQueue &&) = delete;

    private:
    WorkSList workQueue{};
    SharedPtr<KThread> handlerThread{};
};

}
