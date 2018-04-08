#ifndef FUSEE_SDMMC_H
#define FUSEE_SDMMC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t SDHCI_DMA_ADDRESS;
    uint16_t SDHCI_BLOCK_SIZE;
    uint16_t SDHCI_BLOCK_COUNT;
    uint32_t SDHCI_ARGUMENT;
    uint16_t SDHCI_TRANSFER_MODE;
    uint16_t SDHCI_COMMAND;
    uint16_t SDHCI_RESPONSE[0x8];
    uint32_t SDHCI_BUFFER;
    uint32_t SDHCI_PRESENT_STATE;
    uint8_t SDHCI_HOST_CONTROL;
    uint8_t SDHCI_POWER_CONTROL;
    uint8_t SDHCI_BLOCK_GAP_CONTROL;
    uint8_t SDHCI_WAKE_UP_CONTROL;
    uint16_t SDHCI_CLOCK_CONTROL;
    uint8_t SDHCI_TIMEOUT_CONTROL;
    uint8_t SDHCI_SOFTWARE_RESET;
    uint32_t SDHCI_INT_STATUS;
    uint32_t SDHCI_INT_ENABLE;
    uint32_t SDHCI_SIGNAL_ENABLE;
    uint16_t SDHCI_ACMD12_ERR;
    uint16_t SDHCI_HOST_CONTROL2;
    uint32_t SDHCI_CAPABILITIES;
    uint32_t SDHCI_CAPABILITIES_1;
    uint32_t SDHCI_MAX_CURRENT;
    uint32_t _0x4C;
    uint16_t SDHCI_SET_ACMD12_ERROR;
    uint16_t SDHCI_SET_INT_ERROR;
    uint16_t SDHCI_ADMA_ERROR;
    uint8_t _0x55[0x3];
    uint32_t SDHCI_ADMA_ADDRESS;
    uint32_t SDHCI_UPPER_ADMA_ADDRESS;
    uint16_t SDHCI_PRESET_FOR_INIT;
    uint16_t SDHCI_PRESET_FOR_DEFAULT;
    uint16_t SDHCI_PRESET_FOR_HIGH;
    uint16_t SDHCI_PRESET_FOR_SDR12;
    uint16_t SDHCI_PRESET_FOR_SDR25;
    uint16_t SDHCI_PRESET_FOR_SDR50;
    uint16_t SDHCI_PRESET_FOR_SDR104;
    uint16_t SDHCI_PRESET_FOR_DDR50;
    uint8_t _0x70[0x3];
    uint32_t _0x74[0x22];
    uint16_t SDHCI_SLOT_INT_STATUS;
    uint16_t SDHCI_HOST_VERSION;
} sdhci_registers_t;

typedef struct {
    sdhci_registers_t standard_regs;
    uint32_t SDMMC_VENDOR_CLOCK_CNTRL;
    uint32_t SDMMC_VENDOR_SYS_SW_CNTRL;
    uint32_t SDMMC_VENDOR_ERR_INTR_STATUS;
    uint32_t SDMMC_VENDOR_CAP_OVERRIDES;
    uint32_t SDMMC_VENDOR_BOOT_CNTRL;
    uint32_t SDMMC_VENDOR_BOOT_ACK_TIMEOUT;
    uint32_t SDMMC_VENDOR_BOOT_DAT_TIMEOUT;
    uint32_t SDMMC_VENDOR_DEBOUNCE_COUNT;
    uint32_t SDMMC_VENDOR_MISC_CNTRL;
    uint32_t SDMMC_MAX_CURRENT_OVERRIDE;
    uint32_t SDMMC_MAX_CURRENT_OVERRIDE_HI;
    uint32_t _0x12C[0x21];
    uint32_t SDMMC_VENDOR_IO_TRIM_CNTRL;
    /* Start of SDMMC2/SDMMC4 only */
    uint32_t SDMMC_VENDOR_DLLCAL_CFG;
    uint32_t SDMMC_VENDOR_DLL_CTRL0;
    uint32_t SDMMC_VENDOR_DLL_CTRL1;
    uint32_t SDMMC_VENDOR_DLLCAL_CFG_STA;
    /* End of SDMMC2/SDMMC4 only */
    uint32_t SDMMC_VENDOR_TUNING_CNTRL0;
    uint32_t SDMMC_VENDOR_TUNING_CNTRL1;
    uint32_t SDMMC_VENDOR_TUNING_STATUS0;
    uint32_t SDMMC_VENDOR_TUNING_STATUS1;
    uint32_t SDMMC_VENDOR_CLK_GATE_HYSTERESIS_COUNT;
    uint32_t SDMMC_VENDOR_PRESET_VAL0;
    uint32_t SDMMC_VENDOR_PRESET_VAL1;
    uint32_t SDMMC_VENDOR_PRESET_VAL2;
    uint32_t SDMMC_SDMEMCOMPPADCTRL;
    uint32_t SDMMC_AUTO_CAL_CONFIG;
    uint32_t SDMMC_AUTO_CAL_INTERVAL;
    uint32_t SDMMC_AUTO_CAL_STATUS;
    uint32_t SDMMC_IO_SPARE;
    uint32_t SDMMC_SDMMCA_MCCIF_FIFOCTRL;
    uint32_t SDMMC_TIMEOUT_WCOAL_SDMMCA;
    uint32_t _0x1FC;
} sdmmc_registers_t;

static inline volatile sdmmc_registers_t *get_sdmmc1_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0000);
}

static inline volatile sdmmc_registers_t *get_sdmmc1b_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0000 + 0x1000);
}

static inline volatile sdmmc_registers_t *get_sdmmc2_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0200);
}

static inline volatile sdmmc_registers_t *get_sdmmc2b_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0200 + 0x2000);
}

static inline volatile sdmmc_registers_t *get_sdmmc3_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0400);
}

static inline volatile sdmmc_registers_t *get_sdmmc3b_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0400 + 0x3000);
}

static inline volatile sdmmc_registers_t *get_sdmmc4_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0600);
}

static inline volatile sdmmc_registers_t *get_sdmmc4b_regs(void) {
    return (volatile sdmmc_registers_t *)(0x700B0600 + 0x4000);
}

#define SDMMC1_REGS       (get_sdmmc1_regs())
#define SDMMC2_REGS       (get_sdmmc2_regs())
#define SDMMC3_REGS       (get_sdmmc3_regs())
#define SDMMC4_REGS       (get_sdmmc4_regs())

void sdmmc1_init(void);
void sdmmc2_init(void);
void sdmmc3_init(void);
void sdmmc4_init(void);

#endif
