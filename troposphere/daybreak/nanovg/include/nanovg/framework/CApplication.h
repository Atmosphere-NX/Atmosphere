/*
** Sample Framework for deko3d Applications
**   CApplication.h: Wrapper class containing common application boilerplate
*/
#pragma once
#include "common.h"

class CApplication
{
protected:
    virtual void onFocusState(AppletFocusState) { }
    virtual void onOperationMode(AppletOperationMode) { }
    virtual bool onFrame(u64) { return true; }

public:
    CApplication();
    ~CApplication();

    void run();

    static constexpr void chooseFramebufferSize(uint32_t& width, uint32_t& height, AppletOperationMode mode);
};

constexpr void CApplication::chooseFramebufferSize(uint32_t& width, uint32_t& height, AppletOperationMode mode)
{
    switch (mode)
    {
        default:
        case AppletOperationMode_Handheld:
            width = 1280;
            height = 720;
            break;
        case AppletOperationMode_Docked:
            width = 1920;
            height = 1080;
            break;
    }
}
