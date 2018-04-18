#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>

#include "waitablemanager.hpp"
#include "serviceserver.hpp"
#include "ldr_debug_monitor.hpp"
#include "ldr_shell.hpp"

extern u32 __start__;

u32 __nx_applet_type = AppletType_None;

#define INNER_HEAP_SIZE 0x200000
size_t nx_inner_heap_size = INNER_HEAP_SIZE;
char   nx_inner_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void)
{
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

int main(int argc, char **argv)
{
    consoleDebugInit(debugDevice_SVC);
    
    /* TODO: What's a good timeout value to use here? */
    WaitableManager *server_manager = new WaitableManager(U64_MAX);
    
    /* Add services to manager. */
    server_manager->add_waitable(new ServiceServer<ShellService>("dbg:shel", 3));
    server_manager->add_waitable(new ServiceServer<DebugMonitorService>("dbg:dmnt", 2));
    
    /* Loop forever, servicing our services. */
    server_manager->process();
    
    /* Cleanup. */
    delete server_manager;
	return 0;
}

