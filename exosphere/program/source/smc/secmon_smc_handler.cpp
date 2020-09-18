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
#include <exosphere.hpp>
#include "../secmon_error.hpp"
#include "../secmon_misc.hpp"
#include "secmon_smc_common.hpp"
#include "secmon_smc_handler.hpp"
#include "secmon_smc_aes.hpp"
#include "secmon_smc_carveout.hpp"
#include "secmon_smc_device_unique_data.hpp"
#include "secmon_smc_error.hpp"
#include "secmon_smc_info.hpp"
#include "secmon_smc_memory_access.hpp"
#include "secmon_smc_power_management.hpp"
#include "secmon_smc_random.hpp"
#include "secmon_smc_register_access.hpp"
#include "secmon_smc_result.hpp"
#include "secmon_smc_rsa.hpp"

namespace ams::secmon::smc {

    namespace {

        struct HandlerInfo {
            u32 function_id;
            u32 restriction_mask;
            SmcHandler handler;
        };

        struct HandlerTable {
            const HandlerInfo *entries;
            size_t count;
        };

        enum HandlerType : int {
            HandlerType_User  = 0,
            HandlerType_Kern  = 1,
            HandlerType_Count = 2,
        };

        enum Restriction {
            Restriction_None                       = (0 << 0),
            Restriction_Normal                     = (1 << 0),
            Restriction_DeviceUniqueDataNotAllowed = (1 << 1),
            Restriction_SafeModeNotAllowed         = (1 << 2),
        };

        enum SmcCallRange {
            SmcCallRange_ArmArch    = 0,
            SmcCallRange_Cpu        = 1,
            SmcCallRange_Sip        = 2,
            SmcCallRange_Oem        = 3,
            SmcCallRange_Standard   = 4,

            SmcCallRange_TrustedApp = 0x30,
        };

        enum SmcArgumentType {
            ArgumentType_Integer = 0,
            ArgumentType_Pointer = 1,
        };

        enum SmcConvention {
            Convention_Smc32 = 0,
            Convention_Smc64 = 1,
        };

        enum SmcCallType {
            SmcCallType_YieldingCall = 0,
            SmcCallType_FastCall     = 1,
        };

        struct SmcFunctionId {
            using FunctionId    = util::BitPack64::Field< 0,  8, u32>;
            using ArgumentType0 = util::BitPack64::Field< 8,  1, SmcArgumentType>;
            using ArgumentType1 = util::BitPack64::Field< 9,  1, SmcArgumentType>;
            using ArgumentType2 = util::BitPack64::Field<10,  1, SmcArgumentType>;
            using ArgumentType3 = util::BitPack64::Field<11,  1, SmcArgumentType>;
            using ArgumentType4 = util::BitPack64::Field<12,  1, SmcArgumentType>;
            using ArgumentType5 = util::BitPack64::Field<13,  1, SmcArgumentType>;
            using ArgumentType6 = util::BitPack64::Field<14,  1, SmcArgumentType>;
            using ArgumentType7 = util::BitPack64::Field<15,  1, SmcArgumentType>;
            using Reserved      = util::BitPack64::Field<16,  8, u32>;
            using CallRange     = util::BitPack64::Field<24,  6, SmcCallRange>;
            using Convention    = util::BitPack64::Field<30,  1, SmcConvention>;
            using CallType      = util::BitPack64::Field<31,  1, SmcCallType>;
            using Reserved2     = util::BitPack64::Field<32, 32, u32>;
        };

        constinit HandlerInfo g_user_handlers[] = {
            { 0x00000000, Restriction_SafeModeNotAllowed,         nullptr                            },
            { 0xC3000401, Restriction_SafeModeNotAllowed,         SmcSetConfig                       },
            { 0xC3000002, Restriction_Normal,                     SmcGetConfigUser                   },
            { 0xC3000003, Restriction_Normal,                     SmcGetResult                       },
            { 0xC3000404, Restriction_Normal,                     SmcGetResultData                   },
            { 0xC3000E05, Restriction_SafeModeNotAllowed,         SmcModularExponentiate             },
            { 0xC3000006, Restriction_Normal,                     SmcGenerateRandomBytes             },
            { 0xC3000007, Restriction_Normal,                     SmcGenerateAesKek                  },
            { 0xC3000008, Restriction_Normal,                     SmcLoadAesKey                      },
            { 0xC3000009, Restriction_Normal,                     SmcComputeAes                      },
            { 0xC300000A, Restriction_Normal,                     SmcGenerateSpecificAesKey          },
            { 0xC300040B, Restriction_Normal,                     SmcComputeCmac                     },
            { 0xC300D60C, Restriction_Normal,                     SmcReencryptDeviceUniqueData       },
            { 0xC300100D, Restriction_DeviceUniqueDataNotAllowed, SmcDecryptDeviceUniqueData         },
            { 0xC300000E, Restriction_SafeModeNotAllowed,         nullptr                            },
            { 0xC300060F, Restriction_DeviceUniqueDataNotAllowed, SmcModularExponentiateByStorageKey },
            { 0xC3000610, Restriction_SafeModeNotAllowed,         SmcPrepareEsDeviceUniqueKey        },
            { 0xC3000011, Restriction_SafeModeNotAllowed,         SmcLoadPreparedAesKey              },
            { 0xC3000012, Restriction_SafeModeNotAllowed,         SmcPrepareEsCommonTitleKey         }
        };

        /* Deprecated handlerss. */
        constexpr inline const HandlerInfo DecryptAndImportEsDeviceKeyHandlerInfo = {
              0xC300100C, Restriction_Normal, SmcDecryptAndImportEsDeviceKey
        };

        constexpr inline const HandlerInfo DecryptAndImportLotusKeyHandlerInfo = {
              0xC300100E, Restriction_SafeModeNotAllowed, SmcDecryptAndImportLotusKey
        };

        constinit HandlerInfo g_kern_handlers[] = {
            { 0x00000000, Restriction_SafeModeNotAllowed, nullptr },
            { 0xC4000001, Restriction_SafeModeNotAllowed, SmcSuspendCpu                     },
            { 0x84000002, Restriction_SafeModeNotAllowed, SmcPowerOffCpu                    },
            { 0xC4000003, Restriction_SafeModeNotAllowed, SmcPowerOnCpu                     },
            { 0xC3000004, Restriction_Normal,             SmcGetConfigKern                  },
            { 0xC3000005, Restriction_Normal,             SmcGenerateRandomBytesNonBlocking },
            { 0xC3000006, Restriction_Normal,             SmcShowError                      },
            { 0xC3000007, Restriction_Normal,             SmcSetKernelCarveoutRegion        },
            { 0xC3000008, Restriction_Normal,             SmcReadWriteRegister              },

            /* NOTE: Atmosphere extension for mesosphere. This ID is subject to change at any time. */
            { 0xC3000409, Restriction_Normal,             SmcSetConfig                      },
        };

        constinit HandlerInfo g_ams_handlers[] = {
            { 0x00000000, Restriction_SafeModeNotAllowed, nullptr              },
            { 0xF0000201, Restriction_None,               SmcIramCopy          },
            { 0xF0000002, Restriction_None,               SmcReadWriteRegister },
            { 0xF0000003, Restriction_None,               SmcWriteAddress      },
            { 0xF0000404, Restriction_None,               SmcGetEmummcConfig   },
        };

        constexpr const HandlerInfo GetSecureDataHandlerInfo = {
            0x67891234, Restriction_None, SmcGetSecureData
        };

        constinit HandlerTable g_handler_tables[] = {
            { g_user_handlers, util::size(g_user_handlers) },
            { g_kern_handlers, util::size(g_kern_handlers) },
        };

        constinit HandlerTable g_ams_handler_table = {
            g_ams_handlers,  util::size(g_ams_handlers)
        };

        NORETURN void InvalidSmcError(u64 id) {
            SetError(pkg1::ErrorInfo_UnknownSmc);
            AMS_ABORT("Invalid SMC: %lx", id);
        }

        const HandlerTable &GetHandlerTable(HandlerType type, u64 id) {
            /* Ensure we have a valid handler type. */
            if (AMS_UNLIKELY(!(0 <= type && type < HandlerType_Count))) {
                InvalidSmcError(id);
            }

            /* Provide support for legacy SmcGetSecureData. */
            if (id == GetSecureDataHandlerInfo.function_id) {
                return g_handler_tables[HandlerType_User];
            }

            /* Check if we're a user SMC. */
            if (type == HandlerType_User) {
                /* Nintendo uses OEM SMCs. */
                /* We will assign Atmosphere extension SMCs the TrustedApplication range. */
                if (util::BitPack64{id}.Get<SmcFunctionId::CallRange>() == SmcCallRange_TrustedApp) {
                    return g_ams_handler_table;
                }

                /* If we're not performing an atmosphere extension smc, require that we're being invoked by spl on core 3. */
                if (AMS_UNLIKELY(hw::GetCurrentCoreId() != 3)) {
                    InvalidSmcError(id);
                }
            }

            return g_handler_tables[type];
        }

        const HandlerInfo &GetHandlerInfo(const HandlerTable &table, u64 id) {
            /* Provide support for legacy SmcGetSecureData. */
            if (id == GetSecureDataHandlerInfo.function_id) {
                return GetSecureDataHandlerInfo;
            }

            /* Get and check the index. */
            const auto index = util::BitPack64{id}.Get<SmcFunctionId::FunctionId>();
            if (AMS_UNLIKELY(index >= table.count)) {
                InvalidSmcError(id);
            }

            /* Get and check the handler info. */
            const auto &handler_info = table.entries[index];

            /* Check that the handler isn't null. */
            if (AMS_UNLIKELY(handler_info.handler == nullptr)) {
                InvalidSmcError(id);
            }

            /* Check that the handler's id matches. */
            if (AMS_UNLIKELY(handler_info.function_id != id)) {
                InvalidSmcError(id);
            }

            return handler_info;
        }

        bool IsHandlerRestricted(const HandlerInfo &info) {
            return (info.restriction_mask & secmon::GetRestrictedSmcMask()) != 0;
        }

        SmcResult InvokeSmcHandler(const HandlerInfo &info, SmcArguments &args) {
            /* Check if the smc is restricted. */
            if (GetTargetFirmware() >= TargetFirmware_8_0_0 && AMS_UNLIKELY(IsHandlerRestricted(info))) {
                return SmcResult::NotPermitted;
            }

            /* Invoke the smc. */
            return info.handler(args);
        }

    }

    void ConfigureSmcHandlersForTargetFirmware() {
        const auto target_fw = GetTargetFirmware();

        if (target_fw < TargetFirmware_5_0_0) {
            g_user_handlers[DecryptAndImportEsDeviceKeyHandlerInfo.function_id & 0xFF] = DecryptAndImportEsDeviceKeyHandlerInfo;
            g_user_handlers[DecryptAndImportLotusKeyHandlerInfo.function_id & 0xFF]    = DecryptAndImportLotusKeyHandlerInfo;
        }
    }

    void HandleSmc(int type, SmcArguments &args) {
        /* Get the table. */
        const auto &table = GetHandlerTable(static_cast<HandlerType>(type), args.r[0]);

        /* Get the handler info. */
        const auto &info = GetHandlerInfo(table, args.r[0]);

        /* Set the invocation result. */
        args.r[0] = static_cast<u64>(InvokeSmcHandler(info, args));
    }

}
