/*
** Sample Framework for deko3d Applications
**   CApplication.cpp: Wrapper class containing common application boilerplate
*/
#include "CApplication.h"

CApplication::CApplication()
{
    appletLockExit();
    appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);
}

CApplication::~CApplication()
{
    appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleep);
    appletUnlockExit();
}

void CApplication::run()
{
    u64 tick_ref = armGetSystemTick();
    u64 tick_saved = tick_ref;
    bool focused = appletGetFocusState() == AppletFocusState_Focused;

    onOperationMode(appletGetOperationMode());

    for (;;)
    {
        u32 msg = 0;
        Result rc = appletGetMessage(&msg);
        if (R_SUCCEEDED(rc))
        {
            bool should_close = !appletProcessMessage(msg);
            if (should_close)
                return;

            switch (msg)
            {
                case AppletMessage_FocusStateChanged:
                {
                    bool old_focused = focused;
                    AppletFocusState state = appletGetFocusState();
                    focused = state == AppletFocusState_Focused;

                    onFocusState(state);
                    if (focused == old_focused)
                        break;
                    if (focused)
                    {
                        appletSetFocusHandlingMode(AppletFocusHandlingMode_NoSuspend);
                        tick_ref += armGetSystemTick() - tick_saved;
                    }
                    else
                    {
                        tick_saved = armGetSystemTick();
                        appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleepNotify);
                    }
                    break;
                }
                case AppletMessage_OperationModeChanged:
                    onOperationMode(appletGetOperationMode());
                    break;
            }
        }

        if (focused && !onFrame(armTicksToNs(armGetSystemTick() - tick_ref)))
            break;
    }
}
