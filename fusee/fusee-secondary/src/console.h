#ifndef FUSEE_CONSOLE_H
#define FUSEE_CONSOLE_H

int console_init(void);
int console_display(const void *framebuffer); /* Must be page-aligned */
int console_resume(void);
int console_end(void);

#endif
