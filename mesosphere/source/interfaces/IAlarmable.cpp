#include <mesosphere/interfaces/IAlarmable.hpp>
#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/interrupts/KAlarm.hpp>

namespace mesosphere
{

void IAlarmable::SetAlarmTimeImpl(const KSystemClock::time_point &alarmTime)
{
    this->alarmTime = alarmTime;
    KCoreContext::GetCurrentInstance().GetAlarm()->AddAlarmable(*this);
}

void IAlarmable::ClearAlarm()
{
    KCoreContext::GetCurrentInstance().GetAlarm()->RemoveAlarmable(*this);
    alarmTime = KSystemClock::time_point{};
}

}
