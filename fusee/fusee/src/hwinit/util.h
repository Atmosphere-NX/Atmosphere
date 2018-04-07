#ifndef _UTIL_H_
#define _UTIL_H_

#include "types.h"
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wunused-variable"

typedef struct _cfg_op_t
{
	u32 off;
	u32 val;
} cfg_op_t;

void sleep(u32 ticks);
void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops);

#endif
