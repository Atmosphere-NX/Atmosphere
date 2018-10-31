#pragma once

#include <array>
#include <mesosphere/core/util.hpp>

namespace mesosphere
{

class KProcess;
class KThread;
class KScheduler;
class KAlarm;

class KCoreContext {
    public:
    static KCoreContext &GetInstance(uint coreId) { return instances[coreId]; };
    static KCoreContext &GetCurrentInstance() { return instances[0]; /* FIXME*/ };

    KThread *GetCurrentThread() const { return currentThread; }
    KProcess *GetCurrentProcess() const { return currentProcess; }
    KScheduler *GetScheduler() const { return scheduler; }
    KAlarm *GetAlarm() const { return alarm; }

    KCoreContext(KScheduler *scheduler) : scheduler(scheduler) {}
    private:
    KThread *volatile currentThread = nullptr;
    KProcess *volatile currentProcess = nullptr;
    KScheduler *volatile scheduler = nullptr;
    KAlarm *volatile alarm = nullptr;

    // more stuff

    static std::array<KCoreContext, MAX_CORES> instances;
};

}
