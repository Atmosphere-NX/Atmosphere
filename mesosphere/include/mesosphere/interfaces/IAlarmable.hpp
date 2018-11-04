#pragma once

#include <boost/intrusive/set.hpp>
#include <mesosphere/board/KSystemClock.hpp>

namespace mesosphere
{

struct KAlarm;

struct AlarmableSetTag;

using AlarmableSetBaseHook = boost::intrusive::set_base_hook<
    boost::intrusive::tag<AlarmableSetTag>,
    boost::intrusive::link_mode<boost::intrusive::normal_link>
>;

class IAlarmable : public AlarmableSetBaseHook {
    public:
    struct Comparator {
        constexpr bool operator()(const IAlarmable &lhs, const IAlarmable &rhs) const {
            return lhs.alarmTime < rhs.alarmTime;
        }
    };

    virtual void OnAlarm() = 0;

    constexpr KSystemClock::time_point GetAlarmTime() const { return alarmTime; }

    /// Precondition: alarm has not been set
    template<typename Clock, typename Duration>
    void SetAlarmTime(const std::chrono::time_point<Clock, Duration> &alarmTime)
    {
        SetAlarmTimeImpl(alarmTime);
    }

    template<typename Rep, typename Period>
    void SetAlarmIn(const std::chrono::duration<Rep, Period> &alarmTimeOffset)
    {
        SetAlarmTimeImpl(KSystemClock::now() + alarmTimeOffset);
    }

    void ClearAlarm();

    private:
    void SetAlarmTimeImpl(const KSystemClock::time_point &alarmTime);

    KSystemClock::time_point alarmTime = KSystemClock::time_point{};

    friend class KAlarm;
};


using AlarmableSetType =
    boost::intrusive::make_set<
        IAlarmable,
        boost::intrusive::base_hook<AlarmableSetBaseHook>,
        boost::intrusive::compare<IAlarmable::Comparator>
>::type;

}
