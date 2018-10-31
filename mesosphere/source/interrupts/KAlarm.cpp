#include <mesosphere/interrupts/KAlarm.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/arch/KInterruptMaskGuard.hpp>

namespace mesosphere
{

void KAlarm::AddAlarmable(IAlarmable &alarmable)
{
    std::lock_guard guard{spinlock};
    alarmables.insert(alarmable);

    KSystemClock::SetAlarm(alarmables.cbegin()->GetAlarmTime());
}

void KAlarm::RemoveAlarmable(const IAlarmable &alarmable)
{
    std::lock_guard guard{spinlock};
    alarmables.erase(alarmable);

    KSystemClock::SetAlarm(alarmables.cbegin()->GetAlarmTime());
}

void KAlarm::HandleAlarm()
{
    {
        KCriticalSection &critsec = KScheduler::GetCriticalSection();
        std::lock_guard criticalSection{critsec};
        std::lock_guard guard{spinlock};

        KSystemClock::SetInterruptMasked(true); // mask timer interrupt
        KSystemClock::time_point currentTime = KSystemClock::now(), maxAlarmTime;
        while (alarmables.begin() != alarmables.end()) {
            IAlarmable &a = *alarmables.begin();
            maxAlarmTime = a.alarmTime;
            if (maxAlarmTime > currentTime) {
                break;
            }

            alarmables.erase(a);
            a.alarmTime = KSystemClock::time_point{};

            a.OnAlarm();
        }

        if (maxAlarmTime > KSystemClock::time_point{}) {
            KSystemClock::SetAlarm(maxAlarmTime);
        }
    }

    {
        // TODO Reenable interrupt 30
        KInterruptMaskGuard guard{};
    }
}

}
