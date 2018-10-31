#include <cstddef>

void *operator new(std::size_t) { for(;;); }
void *operator new[](std::size_t) { for(;;); }
void operator delete(void *) { }
void operator delete[](void *) { }
