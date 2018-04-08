#include "fuse.h"
#include "t210.h"

void fuse_disable_program()
{
	FUSE(FUSE_DISABLEREGPROGRAM) = 1;
}

u32 fuse_read_odm(u32 idx)
{
	return FUSE(FUSE_RESERVED_ODMX(idx));
}
