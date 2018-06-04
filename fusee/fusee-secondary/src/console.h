#ifndef FUSEE_CONSOLE_H
#define FUSEE_CONSOLE_H

int console_init(bool display_initialized);
void *console_get_framebuffer(bool enable_display);
int console_display(const void *framebuffer); /* Must be page-aligned */
int console_resume(void);
int console_end(void);

#endif
