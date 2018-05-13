#ifndef FUSEE_SPLASH_SCREEN_H
#define FUSEE_SPLASH_SCREEN_H

#include <stdint.h>

extern uint8_t default_splash_bmp[];
extern uint32_t default_splash_bmp_size;

void display_splash_screen_bmp(const char *custom_splash_path, uint8_t *fb_addr);
uint8_t *load_default_splash();

#endif
