#ifndef _HWINIT_H_
#define _HWINIT_H_

void mc_config_tsec_carveout(u32 bom, u32 size1mb, int lock);
void mc_enable_ahb_redirect();
void mc_disable_ahb_redirect();
void nx_hwinit();

#endif
