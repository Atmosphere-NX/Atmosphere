// #include "led.hpp"
// #include <string.h>
// #include <switch.h>

// #include "util.h"

// Breathing effect LED pattern
static const HidsysNotificationLedPattern breathing_pattern = {
    .baseMiniCycleDuration = 0x8, // 100ms.
    .totalMiniCycles = 0x2,       // 3 mini cycles. Last one 12.5ms.
    .totalFullCycles = 0x5,       // 5 full cycles.
    .startIntensity = 0x2,        // 13%.
    .miniCycles = {
        // First cycle
        {
            .ledIntensity = 0xF,      // 100%.
            .transitionSteps = 0xF,   // 15 steps. Transition time 1.5s.
            .finalStepDuration = 0x0, // 12.5ms.
        },
        // Second cycle
        {
            .ledIntensity = 0x2,      // 13%.
            .transitionSteps = 0xF,   // 15 steps. Transition time 1.5s.
            .finalStepDuration = 0x0, // 12.5ms.
        },
    },
};

// Double click LED pattern.
static const HidsysNotificationLedPattern double_click_pattern = {
    .baseMiniCycleDuration = 0x6, // 75ms.
    .totalMiniCycles = 0x2,       // 3 mini cycles. Last one 12.5ms.
    .totalFullCycles = 0x1,       // 1 full cycle.
    .startIntensity = 0xF,        // 100%.
    .miniCycles = {
        // First cycle
        {
            .ledIntensity = 0x0,      // 0%.
            .transitionSteps = 0x0,   // Instant transition time.
            .finalStepDuration = 0x0, // 12.5ms.
        },
        // Second cycle
        {
            .ledIntensity = 0xF,      // 100%.
            .transitionSteps = 0x0,   // Instant transition time.
            .finalStepDuration = 0x1, // 75ms.
        },
    },
};

// Single click LED pattern.
static const HidsysNotificationLedPattern single_click_pattern = {
    .baseMiniCycleDuration = 0x8, // 100ms.
    .totalMiniCycles = 0x1,       // 2 mini cycles. Last one 12.5ms.
    .totalFullCycles = 0x1,       // 1 full cycle.
    .startIntensity = 0xF,        // 100%.
    .miniCycles = {
        // First cycle
        {
            .ledIntensity = 0x0,      // 0%.
            .transitionSteps = 0x5,   // 3 steps. Transition time 0.5s.
            .finalStepDuration = 0x0, // 12.5ms.
        },
    },
};
static bool init_pad = true;
static PadState pad = {0};
static void send_led_pattern(const HidsysNotificationLedPattern* pattern) {
    if (init_pad) {
        // padConfigureInput(2, HidNpadStyleSet_NpadStandard);
        padInitializeAny(&pad);
        hidsysInitialize();
        init_pad = false;
    }
    s32 total_entries;
    HidsysUniquePadId uniquePadIds[4];

    // const Result rc = 
    hidsysGetUniquePadIds(uniquePadIds, 4, &total_entries);
    // if (R_FAILED(rc) && rc != MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer))
    //     fatalThrow(rc);

    for (int i = 0; i < total_entries; i++)
        hidsysSetNotificationLedPattern(pattern, uniquePadIds[i]);
}

void flash_led_connect()
{
    send_led_pattern(&breathing_pattern);
}

void flash_led_disconnect()
{
    HidsysNotificationLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));

    send_led_pattern(&pattern);
}

void flash_led_pause()
{
    send_led_pattern(&double_click_pattern);
}

void flash_led_unpause()
{
    send_led_pattern(&single_click_pattern);
}
