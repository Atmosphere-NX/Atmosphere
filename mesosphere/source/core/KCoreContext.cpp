#include <mesosphere/core/KCoreContext.hpp>
#include <mesosphere/threading/KScheduler.hpp>

namespace mesosphere
{

static KScheduler scheds[4];

std::array<KCoreContext, MAX_CORES> KCoreContext::instances{ &scheds[0], &scheds[1], &scheds[2], &scheds[3] };

}
