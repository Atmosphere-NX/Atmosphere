#pragma once

#include <mesosphere/core/util.hpp>
#include <boost/intrusive/slist.hpp>

namespace mesosphere
{

struct WorkSListTag;

using WorkSListBaseHook = boost::intrusive::slist_base_hook<
    boost::intrusive::tag<WorkSListTag>,
    boost::intrusive::link_mode<boost::intrusive::normal_link>
>;

/// Bottom half in Linux jargon
class IWork : public WorkSListBaseHook {
    public:
    virtual void DoWork() = 0;
};

using WorkSList = boost::intrusive::make_slist<
    IWork,
    boost::intrusive::base_hook<WorkSListBaseHook>,
    boost::intrusive::cache_last<true>,
    boost::intrusive::constant_time_size<false>
>::type;

}
