#pragma once

#include <mesosphere/interfaces/IInterruptibleWork.hpp>
#include <mesosphere/interfaces/IAlarmable.hpp>
#include <mesosphere/interrupts/KInterruptSpinLock.hpp>
#include <mesosphere/board/KSystemClock.hpp>

namespace mesosphere
{

class KAlarm final : public IInterruptibleWork {
    public:

    //KAlarm() = default;

    /// Precondition: alarmable not already added
    void AddAlarmable(IAlarmable &alarmable);

    /// Precondition: alarmable is present
    void RemoveAlarmable(const IAlarmable &alarmable);

    void HandleAlarm();

    KAlarm(const KAlarm &) = delete;
    KAlarm(KAlarm &&) = delete;
    KAlarm &operator=(const KAlarm &) = delete;
    KAlarm &operator=(KAlarm &&) = delete;

    private:
    mutable KInterruptSpinLock<false> spinlock{};
    AlarmableSetType alarmables{};
};

}
