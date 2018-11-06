#include <mesosphere/core/make_object.hpp>
#include <mesosphere/processes/KEvent.hpp>

using namespace mesosphere;

int main(void) {
    auto obj = MakeObjectRaw<KEvent>();
    (void)obj;
    for(;;);
    return 0;
}

extern "C" {
    void _start(void) {
        main();
    }
}
