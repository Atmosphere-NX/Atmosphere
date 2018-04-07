#include "utils.h"
#include "hwinit.h"
#include "loader.h"


int main(void) {
    entrypoint_t entrypoint;
    
    /* TODO: What other hardware init should we do here? */
    
    /* This will load all remaining binaries off of the SD. */
    entrypoint = load_payload();
    
    /* TODO: What do we want to do in terms of argc/argv? */
    entrypoint(0, NULL);
    return 0;
}

