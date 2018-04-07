#include "cluster.h"
#include "i2c.h"
#include "clock.h"
#include "util.h"
#include "pmc.h"
#include "t210.h"

void _cluster_enable_power()
{
	u8 tmp;

	if (i2c_recv_buf_small(&tmp, 1, I2C_5, 0x3C, 0x40))
	{
		tmp &= 0xDFu;
		i2c_send_byte(I2C_5, 0x3C, 0x40, tmp);
	}
	i2c_send_byte(I2C_5, 0x3C, 0x3B, 0x09);

	//Enable cores power.
	i2c_send_byte(I2C_5, 0x1B, 0x02, 0x20);
	i2c_send_byte(I2C_5, 0x1B, 0x03, 0x8D);
	i2c_send_byte(I2C_5, 0x1B, 0x00, 0xB7);
	i2c_send_byte(I2C_5, 0x1B, 0x01, 0xB7);
}

int _cluster_pmc_enable_partition(u32 part, u32 toggle)
{
	//Check if the partition has already been turned on.
	if (PMC(APBDEV_PMC_PWRGATE_STATUS) & part)
		return 0;

	u32 i = 5001;
	while (PMC(APBDEV_PMC_PWRGATE_TOGGLE) & 0x100)
	{
		sleep(1);
		i--;
		if (i < 1)
			return 0;
	}

	PMC(APBDEV_PMC_PWRGATE_TOGGLE) = toggle | 0x100;

	i = 5001;
	while (i > 0)
	{
		if (PMC(APBDEV_PMC_PWRGATE_STATUS) & part)
			break;
		sleep(1);
		i--;
	}

	return 1;
}

void cluster_enable_cpu0(u64 entry, u32 ns_disable)
{
	//Set ACTIVE_CLUSER to FAST.
	FLOW_CTLR(FLOW_CTLR_BPMP_CLUSTER_CONTROL) &= 0xFFFFFFFE;

	_cluster_enable_power();

	if (!(CLOCK(0xE0) & 0x40000000))
	{
		CLOCK(0x518) &= 0xFFFFFFF7;
		sleep(2);
		CLOCK(0xE4) = CLOCK(0xE4) & 0xFFFBFFFF | 0x40000;
		CLOCK(0xE0) = 0x40404E02;
	}
	while (!(CLOCK(0xE0) & 0x8000000))
		;

	CLOCK(0x3B4) = CLOCK(0x3B4) & 0x1FFFFF00 | 6;
	CLOCK(0x360) = CLOCK(0x360) & 0xFFFFFFF7 | 8;
	CLOCK(0x20) = 0x20008888;
	CLOCK(0x24) = 0x80000000;
	CLOCK(0x440) = 1;

	clock_enable_coresight();

	CLOCK(0x388) = CLOCK(0x388) & 0xFFFFE000;

	//Enable CPU rail.
	_cluster_pmc_enable_partition(1, 0);
	//Enable cluster 0 non-CPU.
	_cluster_pmc_enable_partition(0x8000, 15);
	//Enable CE0.
	_cluster_pmc_enable_partition(0x4000, 14);

	//Request and wait for RAM repair.
	FLOW_CTLR(FLOW_CTLR_RAM_REPAIR) = 1;
	while (!(FLOW_CTLR(FLOW_CTLR_RAM_REPAIR) & 2))
		;

	EXCP_VEC(0x100) = 0;

	if(ns_disable)
	{
		//Set reset vectors.
		SB(SB_AA64_RESET_LOW) = (u32)entry | 1;
		SB(SB_AA64_RESET_HIGH) = (u32)(entry >> 32);
		//Non-secure reset vector write disable.
		SB(SB_CSR_0) = 2;
	}
	else
	{
		//Set reset vectors.
		SB(SB_AA64_RESET_LOW) = (u32)entry;
		SB(SB_AA64_RESET_HIGH) = (u32)(entry >> 32);
	}

	//Until here the CPU was in reset, this kicks execution.
	CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_V) &= 0xFFFFFFF7;
	CLOCK(CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR) = 0x411F000F;
}
