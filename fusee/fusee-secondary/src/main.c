#include "utils.h"
#include "hwinit.h"
#include "loader.h"

#include "stage2.h"

/* Allow for main(int argc, void **argv) signature. */
#pragma GCC diagnostic ignored "-Wmain"

int main(int argc, void **argv) {
    entrypoint_t entrypoint;
    
    /* TODO: What other hardware init should we do here? */
    
    /* This will load all remaining binaries off of the SD. */
    entrypoint = load_payload();
    
    /* TODO: What do we want to do in terms of argc/argv? */
    entrypoint(0, NULL);
    return 0;
}

