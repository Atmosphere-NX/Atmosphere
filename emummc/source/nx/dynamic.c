/**
 * @file dynamic.c
 * @copyright libnx Authors
 */

#include <stddef.h>
#include "../utils/types.h"
#include <elf.h>

void __nx_dynamic(uintptr_t base, const Elf64_Dyn* dyn)
{
	const Elf64_Rela* rela = NULL;
	u64 relasz = 0;

	for (; dyn->d_tag != DT_NULL; dyn++)
	{
		switch (dyn->d_tag)
		{
			case DT_RELA:
				rela = (const Elf64_Rela*)(base + dyn->d_un.d_ptr);
				break;
			case DT_RELASZ:
				relasz = dyn->d_un.d_val / sizeof(Elf64_Rela);
				break;
		}
	}

	if (rela == NULL) {
        while(true)
            ;
    }

	for (; relasz--; rela++)
	{
		switch (ELF64_R_TYPE(rela->r_info))
		{
			case R_AARCH64_RELATIVE:
			{
				u64* ptr = (u64*)(base + rela->r_offset);
				*ptr = base + rela->r_addend;
				break;
			}
		}
	}
}
