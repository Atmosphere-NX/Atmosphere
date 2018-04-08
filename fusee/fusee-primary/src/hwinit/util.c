#include "util.h"
#include "t210.h"

void sleep(u32 ticks)
{
	u32 start = TMR(0x10);
	while (TMR(0x10) - start <= ticks)
		;
}

void exec_cfg(u32 *base, const cfg_op_t *ops, u32 num_ops)
{
	for(u32 i = 0; i < num_ops; i++)
		base[ops[i].off] = ops[i].val;
}

