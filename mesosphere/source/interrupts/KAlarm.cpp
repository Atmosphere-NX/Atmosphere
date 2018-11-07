#include <mesosphere/interrupts/KAlarm.hpp>
#include <mesosphere/threading/KScheduler.hpp>
#include <mesosphere/arch/KInterruptMaskGuard.hpp>

namespace mesosphere
{

void KAlarm::AddAlarmable(IAlarmable &alarmable)
{
    std::scoped_lock guard{spinlock};
    alarmables.insert(alarmable);

    KSystemClock::SetAlarm(alarmables.cbegin()->GetAlarmTime());
}

void KAlarm::RemoveAlarmable(const IAlarmable &alarmable)
{
    std::scoped_lock guard{spinlock};
    alarmables.erase(alarmable);

    KSystemClock::SetAlarm(alarmables.cbegin()->GetAlarmTime());
}

void KAlarm::HandleAlarm()
{
    {
        KCriticalSection &critsec = KScheduler::GetCriticalSection();
        std::scoped_lock criticalSection{critsec};
        std::scoped_lock guard{spinlock};

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
