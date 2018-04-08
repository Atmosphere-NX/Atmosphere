#ifndef FUSEE_LOADER_H
#define FUSEE_LOADER_H

typedef void (*entrypoint_t)(int argc, const char *argv); 

entrypoint_t load_payload(void);

#endif