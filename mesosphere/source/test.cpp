#include <mesosphere/core/make_object.hpp>
#include <mesosphere/interrupts/KInterruptEvent.hpp>

using namespace mesosphere;

int main(void) {
    auto [res, h1] = MakeObjectWithHandle<KInterruptEvent>(3, -1);
    (void)res; (void)h1; //(void)h2;
    for(;;);
    return 0;
}

extern "C" {
    void _start(void) {
        main();
    }
}
