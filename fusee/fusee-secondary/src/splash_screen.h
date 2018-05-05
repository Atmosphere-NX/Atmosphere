#ifndef FUSEE_SPLASH_SCREEN_H
#define FUSEE_SPLASH_SCREEN_H

#include <stdint.h>

/* TODO: Actually make this a real thing. */
extern unsigned char g_default_splash_screen[1];

void display_splash_screen_bmp(const char *custom_splash_path);

#endif
