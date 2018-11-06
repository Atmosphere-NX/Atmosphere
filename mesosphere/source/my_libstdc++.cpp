#include <cstddef>

void *operator new(std::size_t) { for(;;); }
void *operator new[](std::size_t) { for(;;); }
void operator delete(void *) { }
void operator delete(void *, std::size_t) { }
void operator delete[](void *) { }
void operator delete[](void *, std::size_t) { }

extern "C" {

int
__cxa_atexit (void (*fn) (void *),
	void *arg,
	void *d)
{
  return 0;
}
void *__dso_handle = 0;

[[noreturn]] void __cxa_pure_virtual() { for (;;); }

}