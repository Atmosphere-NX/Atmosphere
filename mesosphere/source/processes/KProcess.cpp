#include <mesosphere/processes/KProcess.hpp>
#include <mesosphere/threading/KThread.hpp>
#include <mesosphere/kresources/KResourceLimit.hpp>

namespace mesosphere
{

void KProcess::SetLastThreadAndIdleSelectionCount(KThread *thread, ulong idleSelectionCount)
{
    lastThreads[thread->GetCurrentCoreId()] = thread;
    lastIdleSelectionCount[thread->GetCurrentCoreId()] = idleSelectionCount;
}

bool KProcess::IsSignaled() const
{
    return stateChanged;
}

void KProcess::SetDebug(KDebug *debug)
{
    this->debug = debug;
    if (state != State::DebugSuspended) {
        state = state == State::Created ? State::CreatedAttached : State::DebugSuspended;
        stateChanged = true;
        NotifyWaiters();
    }
}

void KProcess::ClearDebug(KProcess::State attachState)
{
    debug = nullptr;
    State oldState = state;
    if (state == State::StartedAttached || state == State::DebugSuspended) {
        state = attachState == State::Created ? State::Started : attachState; // Restore the old state
    } else if (state == State::CreatedAttached) {
        state = State::Created;
    }

    if (state != oldState) {
        stateChanged = true;
        NotifyWaiters();
    }
}

void KProcess::SetDebugPauseState(bool pause) {
    if (pause && state == State::StartedAttached) {
        state = State::DebugSuspended;
    } else if (!pause && state == State::DebugSuspended) {
        state = State::StartedAttached;
    } else {
        return;
    }

    stateChanged = true;
    NotifyWaiters();
}

}
