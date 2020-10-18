/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xusb.h"

#include "xusb_event_ring.h"
#include "xusb_endpoint.h"

#include "../reg/reg_apbdev_pmc.h"
#include "../reg/reg_car.h"
#include "../reg/reg_fuse.h"
#include "../reg/reg_xusb_padctl.h"
#include "../reg/reg_xusb_dev.h"

extern "C" {
#include "../timers.h"
#include "../lib/log.h"
#include "../utils.h"
#include "../mc.h"
}

#include<array>

namespace ams {

    namespace xusb {
    
        namespace impl {
        
            namespace padctl {

                void SetupPads(t210::CLK_RST_CONTROLLER::OscFreq osc_freq);
                void SetupPllu(t210::CLK_RST_CONTROLLER::OscFreq osc_freq);
                void SetupUTMIP(t210::CLK_RST_CONTROLLER::OscFreq osc_freq);
                void ApplyFuseCalibration();
                void DisablePd();
                void SetupTracking(t210::CLK_RST_CONTROLLER::OscFreq osc_freq);

            }

            void ConfigureClockAndResetForDeviceMode();

            EventRing event_ring;
            TRB ep0_transfer_ring[16];
        
            Gadget *current_gadget = nullptr;
        }
    
        void Initialize() {
            const bool use_bootrom = false;
        
            t210::CLK_RST_CONTROLLER
                .CLK_OUT_ENB_W_0()
                .CLK_ENB_XUSB.Enable()
                .Commit()
                .RST_DEVICES_W_0()
                .SWR_XUSB_PADCTL_RST.Set(0)
                .Commit();
    
            udelay(2);

            t210::XUSB_PADCTL
                .USB2_PAD_MUX_0()
                .USB2_OTG_PAD_PORT0.SetXUSB()
                .USB2_BIAS_PAD.SetXUSB()
                .Commit();

            if (use_bootrom) {
                ((bool (*)()) (0x1104fd))();
            } else {
                impl::padctl::SetupPads(t210::CLK_RST_CONTROLLER.OSC_CTRL_0().OSC_FREQ.Get());
            }

            t210::XUSB_PADCTL
                .USB2_PORT_CAP_0()
                .PORT0_CAP.SetDeviceOnly()
                .Commit()
                .SS_PORT_MAP_0()
                .PORT0_MAP.SetUSB2Port0()
                .Commit();

            t210::APBDEV_PMC
                .USB_AO_0()
                .VBUS_WAKEUP_PD_P0.Set(0)
                .ID_PD_P0.Set(0)
                .Commit();

            udelay(1);

            if (use_bootrom) {
                ((void (*)()) (0x110227))();
            } else {
                impl::ConfigureClockAndResetForDeviceMode();
            }
        
            impl::event_ring.Initialize();

            endpoints[0].InitializeForControl(impl::ep0_transfer_ring, std::size(impl::ep0_transfer_ring));

            t210::T_XUSB_DEV_XHCI
                .ECPLO()
                .ADDRLO.Set(endpoints.GetContextsForHardware())
                .Commit()
                .ECPHI()
                .ADDRHI.Set(0)
                .Commit();

            endpoints[0].Reload();
        
            t210::T_XUSB_DEV_XHCI
                .CTRL()
                .LSE.Enable()
                .IE.Enable()
                .Commit();
        }

        void EnableDevice(Gadget &gadget) {
            impl::current_gadget = &gadget;
        
            t210::XUSB_PADCTL
                .ELPG_PROGRAM_0_0(0).Commit()
                .ELPG_PROGRAM_1_0(0).Commit()
                .USB2_VBUS_ID_0()
                .VBUS_SOURCE_SELECT.SetOVERRIDE()
                .ID_SOURCE_SELECT.SetOVERRIDE()
                .Commit();

            t210::T_XUSB_DEV_XHCI
                .PORTHALT()
                .HALT_LTSSM.Set(0)
                .Commit()
                .CTRL()
                .ENABLE.Enable()
                .Commit()
                .CFG_DEV_FE()
                .PORTREGSEL.SetHSFS() // PortSC accesses route to register for HS
                .Commit()
                .PORTSC()
                .LWS.Set(1)
                .PLS.SetRXDETECT()
                .Commit()
                .CFG_DEV_FE()
                .PORTREGSEL.SetSS() // PortSC accesses route to register for SS
                .Commit()
                .PORTSC()
                .LWS.Set(1)
                .PLS.SetRXDETECT()
                .Commit()
                .CFG_DEV_FE()
                .PORTREGSEL.SetINIT() // PortSC Accesses route to register based on current link speed
                .Commit();

            t210::XUSB_PADCTL
                .USB2_VBUS_ID_0()
                .VBUS_OVERRIDE.Set(1)
                .ID_OVERRIDE.SetFLOAT()
                .Commit();
        }

        void Process() {
            impl::event_ring.Process();
        }

        void Finalize() {
            mc_release_ahb_redirect();
        }

        Gadget *GetCurrentGadget() {
            return impl::current_gadget;
        }
    
        namespace impl {
        
            namespace padctl {
    
                void SetupPads(t210::CLK_RST_CONTROLLER::OscFreq osc_freq) {
                    constexpr bool use_bootrom = false;
        
                    SetupPllu(osc_freq);

                    if (use_bootrom) {
                        ((void (*)(t210::CLK_RST_CONTROLLER::OscFreq)) (0x11039b))(osc_freq);
                        ((void (*)()) (0x110357))();
                        ((int (*)()) (0x110327))();
                        ((bool (*)(t210::CLK_RST_CONTROLLER::OscFreq)) (0x1102cb))(osc_freq);
                    } else {
                        SetupUTMIP(osc_freq);
                        ApplyFuseCalibration();
                        DisablePd();
                        SetupTracking(osc_freq);
                    }

                    udelay(30);
                }

                void SetupPllu(t210::CLK_RST_CONTROLLER::OscFreq osc_freq) {
                    struct {
                        uint8_t divn, divm, divp;
                    } pllu_config;

                    using OscFreq = t210::CLK_RST_CONTROLLER::OscFreq;
        
                    switch(osc_freq) {
                    case OscFreq::OSC13:
                        pllu_config = {37, 1, 1};
                        break;
                    case OscFreq::OSC16P8:
                        pllu_config = {28, 1, 1};
                        break;
                    case OscFreq::OSC19P2:
                        pllu_config = {25, 1, 1};
                        break;
                    case OscFreq::OSC38P4:
                        pllu_config = {25, 2, 1};
                        break;
                    case OscFreq::OSC12:
                        pllu_config = {40, 1, 1};
                        break;
                    case OscFreq::OSC48:
                        pllu_config = {40, 4, 1};
                        break;
                    case OscFreq::OSC26:
                        pllu_config = {37, 2, 1};
                        break;
                    default:
                        fatal_error("xusb initialization failed: invalid oscillator frequency");
                        break;
                    }

                    // reset PLLU
                    t210::CLK_RST_CONTROLLER
                        .PLLU_BASE_0()
                        .PLLU_ENABLE.Disable()
                        .PLLU_CLKENABLE_48M.Disable()
                        .PLLU_OVERRIDE.Set(1)
                        .PLLU_CLKENABLE_ICUSB.Disable()
                        .PLLU_CLKENABLE_HSIC.Disable()
                        .PLLU_CLKENABLE_USB.Disable()
                        .Commit();

                    udelay(100);

                    t210::CLK_RST_CONTROLLER
                        .PLLU_MISC_0()
                        .PLLU_EN_LCKDET.Enable()
                        .Commit()
                        .PLLU_BASE_0()
                        .PLLU_DIVM.Set(pllu_config.divm)
                        .PLLU_DIVN.Set(pllu_config.divn)
                        .PLLU_DIVP.Set(pllu_config.divp)
                        .PLLU_OVERRIDE.Set(1)
                        .PLLU_ENABLE.Enable()
                        .Commit();

                    int i = 0;
        
                    while (!t210::CLK_RST_CONTROLLER.PLLU_BASE_0().PLLU_LOCK && i < 100000) {
                        i++;
                    }
        
                    if (t210::CLK_RST_CONTROLLER.PLLU_BASE_0().PLLU_LOCK) {
                        print(SCREEN_LOG_LEVEL_DEBUG, "got PLLU lock in %d iterations\n", i);
                    } else {
                        fatal_error("xusb initialization failed: PLLU lock not acquired in %d iterations\n", i);
                    }

                    t210::CLK_RST_CONTROLLER
                        .PLLU_BASE_0()
                        .PLLU_CLKENABLE_USB.Enable()
                        .PLLU_CLKENABLE_HSIC.Enable()
                        .PLLU_CLKENABLE_48M.Enable()
                        .PLLU_CLKENABLE_ICUSB.Enable()
                        .Commit();
                }

                void SetupUTMIP(t210::CLK_RST_CONTROLLER::OscFreq osc_freq) {
                    t210::CLK_RST_CONTROLLER
                        .UTMIPLL_HW_PWRDN_CFG0_0()
                        .UTMIPLL_IDDQ_SWCTL.Set(1) // "IDDQ by software."
                        .UTMIPLL_IDDQ_OVERRIDE_VALUE.Set(0) // "PLL not in IDDQ mode... Override value used only when UTMIP_LL_IDDQ_SWCTL is set"
                        .Commit();

                    constexpr bool use_pllu = false; // works either way

                    struct {
                        uint8_t ndiv, mdiv;
                    } pll_config;

                    struct {
                        uint16_t enable_dly_count;
                        /* only used with PLLU reference source */
                        uint16_t stable_count;
                        /* only used with PLLU reference source */
                        uint16_t active_dly_count;
                        uint16_t xtal_freq_count;
                    } count_config;

                    using OscFreq = t210::CLK_RST_CONTROLLER::OscFreq;
        
                    switch(osc_freq) {
                    case OscFreq::OSC13:
                        pll_config = {74, 1};
                        count_config = {2, 51, 9, 127};
                        break;
                    case OscFreq::OSC16P8:
                        pll_config = {57, 1};
                        count_config = {3, 66, 11, 165};
                        break;
                    case OscFreq::OSC19P2:
                        pll_config = {50, 1};
                        count_config = {3, 75, 12, 188};
                        break;
                    case OscFreq::OSC38P4:
                        pll_config = {25, 1};
                        count_config = {5, 150, 24, 375};
                        break;
                    case OscFreq::OSC12:
                        pll_config = {80, 1};
                        count_config = {2, 47, 8, 118};
                        break;
                    case OscFreq::OSC48:
                        pll_config = {40, 2};
                        count_config = {6, 188, 31, 469};
                        break;
                    case OscFreq::OSC26:
                        pll_config = {74, 2};
                        count_config = {4, 102, 17, 254};
                        break;
                    default:
                        fatal_error("xusb initialization failed: invalid oscillator frequency");
                        break;
                    }
        
                    if (use_pllu) {
                        t210::CLK_RST_CONTROLLER
                            .UTMIP_PLL_CFG3_0()
                            .UTMIP_PLL_REF_SRC_SEL.Set(1)
                            .Commit();

                        /* 480 MHz * 30 / 15 = 960 MHz */
                        pll_config = {30, 15};
                    } else {
                        // TODECIDE: set PLL_REF_SRC_SEL? or assume correct from reset?

                        /* don't need to wait for PLLU to become stable if we're not using it */
                        count_config.stable_count = 0;
                        count_config.enable_dly_count = 0;
                    }
            
                    t210::CLK_RST_CONTROLLER
                        .UTMIP_PLL_CFG0_0()
                        .UTMIP_PLL_NDIV.Set(pll_config.ndiv)
                        .UTMIP_PLL_MDIV.Set(pll_config.mdiv)
                        .Commit()
                        .UTMIP_PLL_CFG2_0()
                        .UTMIP_PHY_XTAL_CLOCKEN.Set(1)
                        .UTMIP_PLLU_STABLE_COUNT.Set(count_config.stable_count)
                        .UTMIP_PLL_ACTIVE_DLY_COUNT.Set(count_config.active_dly_count)
                        .Commit()
                        .UTMIP_PLL_CFG1_0()
                        .UTMIP_PLLU_ENABLE_DLY_COUNT.Set(count_config.enable_dly_count)
                        .UTMIP_XTAL_FREQ_COUNT.Set(count_config.xtal_freq_count)
                        .UTMIP_FORCE_PLL_ACTIVE_POWERDOWN.Set(0)
                        .UTMIP_FORCE_PLL_ENABLE_POWERUP.Set(1)
                        .UTMIP_FORCE_PLL_ENABLE_POWERDOWN.Set(0)
                        .Commit();

                    for (int i = 0; i < 100 && !t210::CLK_RST_CONTROLLER.UTMIPLL_HW_PWRDN_CFG0_0().UTMIPLL_LOCK; i++) {
                        udelay(1);
                    }

                    if (t210::CLK_RST_CONTROLLER.UTMIPLL_HW_PWRDN_CFG0_0().UTMIPLL_LOCK) {
                        print(SCREEN_LOG_LEVEL_DEBUG, "got UTMIPLL lock\n");
                        print(SCREEN_LOG_LEVEL_DEBUG, "UTMIP_PLL_CFG0_0: 0x%08x\n", t210::CLK_RST_CONTROLLER.UTMIP_PLL_CFG0_0().read_value);
                        print(SCREEN_LOG_LEVEL_DEBUG, "UTMIP_PLL_CFG1_0: 0x%08x\n", t210::CLK_RST_CONTROLLER.UTMIP_PLL_CFG1_0().read_value);
                        print(SCREEN_LOG_LEVEL_DEBUG, "UTMIP_PLL_CFG2_0: 0x%08x\n", t210::CLK_RST_CONTROLLER.UTMIP_PLL_CFG2_0().read_value);
                    } else {
                        /* There is some issue where occasionally after a reboot-to-self we
                         * fail to get a UTMIP lock. The BootROM ignores this failure and
                         * continues on. It seems that in these cases it eventually starts
                         * working, but may take a few seconds to do so. */
                        print(SCREEN_LOG_LEVEL_ERROR, "failed to get UTMIP lock\n");
                        print(SCREEN_LOG_LEVEL_DEBUG, "UTMIP_PLL_CFG0_0: 0x%08x\n", t210::CLK_RST_CONTROLLER.UTMIP_PLL_CFG0_0().read_value);
                        print(SCREEN_LOG_LEVEL_DEBUG, "UTMIP_PLL_CFG1_0: 0x%08x\n", t210::CLK_RST_CONTROLLER.UTMIP_PLL_CFG1_0().read_value);
                        print(SCREEN_LOG_LEVEL_DEBUG, "UTMIP_PLL_CFG2_0: 0x%08x\n", t210::CLK_RST_CONTROLLER.UTMIP_PLL_CFG2_0().read_value);
                    }

                    t210::CLK_RST_CONTROLLER
                        .UTMIP_PLL_CFG2_0()
                        .UTMIP_FORCE_PD_SAMP_A_POWERDOWN.Set(0)
                        .UTMIP_FORCE_PD_SAMP_A_POWERUP.Set(1)
                        .UTMIP_FORCE_PD_SAMP_B_POWERDOWN.Set(0)
                        .UTMIP_FORCE_PD_SAMP_B_POWERUP.Set(1)
                        .UTMIP_FORCE_PD_SAMP_C_POWERDOWN.Set(0)
                        .UTMIP_FORCE_PD_SAMP_C_POWERUP.Set(1)
                        .UTMIP_FORCE_PD_SAMP_D_POWERDOWN.Set(0)
                        .UTMIP_FORCE_PD_SAMP_D_POWERUP.Set(1)
                        .Commit()
                        .UTMIP_PLL_CFG1_0()
                        .UTMIP_FORCE_PLL_ENABLE_POWERUP.Set(0)
                        .UTMIP_FORCE_PLL_ENABLE_POWERDOWN.Set(0)
                        .Commit()
                        .UTMIPLL_HW_PWRDN_CFG0_0()
                        .UTMIPLL_USE_LOCKDET.Set(1)
                        .UTMIPLL_CLK_ENABLE_SWCTL.Set(0)
                        .UTMIPLL_SEQ_ENABLE.Set(1)
                        .Commit();
        
                    udelay(2);
                }

                void ApplyFuseCalibration() {
                    auto fuse = t210::FUSE.SKU_USB_CALIB();

                    t210::XUSB_PADCTL
                        .USB2_OTG_PAD0_CTL_0_0()
                        .HS_CURR_LEVEL.Set(fuse.HS_CURR_LEVEL.Get())
                        .Commit()
                        .USB2_OTG_PAD0_CTL_1_0()
                        .TERM_RANGE_ADJ.Set(fuse.TERM_RANGE_ADJ.Get())
                        .RPD_CTRL.Set(t210::FUSE.USB_CALIB_EXT().RPD_CTRL.Get())
                        .Commit()
                        .USB2_BATTERY_CHRG_OTGPAD0_CTL1_0()
                        .VREG_FIX18.Set(0)
                        .VREG_LEV.Set(1)
                        .Commit();
                }

                void DisablePd() {
                    t210::XUSB_PADCTL
                        .USB2_OTG_PAD0_CTL_0_0()
                        .PD_ZI.Set(0)
                        .PD.Set(0)
                        .Commit()
                        .USB2_OTG_PAD0_CTL_1_0()
                        .PD_DR.Set(0)
                        .Commit()
                        .USB2_BATTERY_CHRG_OTGPAD0_CTL0_0()
                        .PD_CHG.Set(0)
                        .Commit()
                        .USB2_BIAS_PAD_CTL_0_0()
                        .PD.Set(0)
                        .Commit();
                }

                void SetupTracking(t210::CLK_RST_CONTROLLER::OscFreq osc_freq) {
                    uint8_t divisor;

                    using OscFreq = t210::CLK_RST_CONTROLLER::OscFreq;
        
                    switch(osc_freq) {
                    case OscFreq::OSC13:
                        divisor = 2;
                        break;
                    case OscFreq::OSC16P8:
                        divisor = 2;
                        break;
                    case OscFreq::OSC19P2:
                        divisor = 2;
                        break;
                    case OscFreq::OSC38P4:
                        divisor = 6;
                        break;
                    case OscFreq::OSC12:
                        divisor = 2;
                        break;
                    case OscFreq::OSC48:
                        divisor = 6;
                        break;
                    case OscFreq::OSC26:
                        divisor = 4;
                        break;
                    default:
                        fatal_error("xusb initialization failed: invalid oscillator frequency");
                        break;
                    }
        
                    t210::CLK_RST_CONTROLLER
                        .CLK_OUT_ENB_Y_0()
                        .CLK_ENB_USB2_TRK.Enable()
                        .Commit()
                        .CLK_SOURCE_USB2_HSIC_TRK_0()
                        .USB2_HSIC_TRK_CLK_DIVISOR.Set(divisor)
                        .Commit();

                    auto reg = t210::XUSB_PADCTL
                        .USB2_BIAS_PAD_CTL_1_0(0)
                        .TRK_DONE_RESET_TIMER.Set(10)
                        .TRK_START_TIMER.Set(30);

                    reg.PD_TRK.Set(0).Commit();
                    udelay(100);
                    reg.PD_TRK.Set(1).Commit();
                    udelay(3);
                    reg.PD_TRK.Set(0).Commit();
                    udelay(100);

                    t210::XUSB_PADCTL
                        .USB2_BIAS_PAD_CTL_1_0()
                        .PD_TRK.Set(1)
                        .Commit();
                    t210::CLK_RST_CONTROLLER
                        .CLK_OUT_ENB_Y_0()
                        .CLK_ENB_USB2_TRK.Disable()
                        .Commit();
                }

            } // namespace padctl

            void ConfigureClockAndResetForDeviceMode() {
                t210::CLK_RST_CONTROLLER
                    .PLLU_OUTA_0()
                    .PLLU_OUT1_RSTN.Set(1) // RESET_DISABLE
                    .Commit();

                udelay(2);

                t210::CLK_RST_CONTROLLER
                    .CLK_OUT_ENB_U_0()
                    .CLK_ENB_XUSB_DEV.Enable()
                    .Commit()
                    .CLK_SOURCE_XUSB_CORE_DEV_0()
                    .XUSB_CORE_DEV_CLK_SRC.SetPLLP_OUT0()
                    .XUSB_CORE_DEV_CLK_DIVISOR.Set(6)
                    .Commit();

                udelay(2);

                t210::CLK_RST_CONTROLLER
                    .CLK_SOURCE_XUSB_FS_0()
                    .XUSB_FS_CLK_SRC.SetFO_48M()
                    .Commit()
                    .CLK_OUT_ENB_W_0()
                    .CLK_ENB_XUSB_SS.Enable()
                    .Commit()
                    .CLK_SOURCE_XUSB_SS_0()
                    .XUSB_SS_CLK_SRC.SetHSIC_480()
                    .XUSB_SS_CLK_DIVISOR.Set(6)
                    .Commit()
                    .RST_DEVICES_W_0()
                    .SWR_XUSB_SS_RST.Set(0) // DISABLE
                    .Commit()
                    .RST_DEVICES_U_0()
                    .SWR_XUSB_DEV_RST.Set(0) // DISABLE
                    .Commit();

                udelay(2);

                mc_acquire_ahb_redirect();

                t210::XUSB_DEV
                    .CONFIGURATION_0()
                    .EN_FPCI.Enable()
                    .Commit();

                t210::T_XUSB_DEV
                    .CFG_1()
                    .IO_SPACE.Enable()
                    .MEMORY_SPACE.Enable()
                    .BUS_MASTER.Enable()
                    .Commit();

                udelay(1);

                t210::T_XUSB_DEV
                    .CFG_4(0)
                    .BASE_ADDRESS.Set(t210::XUSB_DEV_BASE)
                    .PREFETCHABLE.SetNOT()
                    .ADDRESS_TYPE.Set32_BIT()
                    .SPACE_TYPE.SetMEMORY()
                    .Commit();

                t210::XUSB_DEV
                    .INTR_MASK_0()
                    .IP_INT_MASK.Set(1) // "IP (SATA/AZ) interrupt to MPCORE gated by mask."
                    .Commit();
            }

        } // namespace impl

        Result xusb::Gadget::HandleSetupRequest(usb::SetupPacket &packet) {
            return ResultUnknownSetupRequest();
        }
        
    } // namespace xusb

} // namespace ams
