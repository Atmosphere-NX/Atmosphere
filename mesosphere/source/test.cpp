#include <mesosphere/core/make_object.hpp>
#include <mesosphere/processes/KEvent.hpp>

using namespace mesosphere;

int main(void) {
    auto [res, h1, h2] = MakeObjectWithHandle<KEvent>();
    (void)res; (void)h1; (void)h2;
    for(;;);
    return 0;
}

extern "C" {
    void _start(void) {
        main();
    }
}
