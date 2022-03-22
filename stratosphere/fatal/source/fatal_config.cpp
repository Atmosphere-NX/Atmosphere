/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "fatal_config.hpp"

namespace ams::fatal::srv {

    namespace {

        /* Global config. */
        constinit os::SdkMutex g_config_mutex;
        constinit bool g_initialized_config;
        constinit util::TypedStorage<FatalConfig> g_config;

        FatalConfig &GetFatalConfigImpl() {
            if (AMS_UNLIKELY(!g_initialized_config)) {
                std::scoped_lock lk(g_config_mutex);

                if (AMS_LIKELY(!g_initialized_config)) {
                    util::ConstructAt(g_config);

                    g_initialized_config = true;
                }
            }

            return util::GetReference(g_config);
        }


        /* Event creator. */
        os::NativeHandle GetFatalDirtyEventReadableHandle() {
            Event evt;
            R_ABORT_UNLESS(setsysAcquireFatalDirtyFlagEventHandle(std::addressof(evt)));
            return evt.revent;
        }

        /* Global event. */
        constinit os::SystemEventType g_fatal_dirty_event;
        constinit os::MultiWaitHolderType g_fatal_dirty_multi_wait_holder;
        constinit bool g_initialized_fatal_dirty_event;

    }

    os::MultiWaitHolderType *GetFatalDirtyMultiWaitHolder() {
        if (AMS_UNLIKELY(!g_initialized_fatal_dirty_event)) {
            os::AttachReadableHandleToSystemEvent(std::addressof(g_fatal_dirty_event), GetFatalDirtyEventReadableHandle(), true, os::EventClearMode_ManualClear);
            os::InitializeMultiWaitHolder(std::addressof(g_fatal_dirty_multi_wait_holder), std::addressof(g_fatal_dirty_event));
            os::SetMultiWaitHolderUserData(std::addressof(g_fatal_dirty_multi_wait_holder), reinterpret_cast<uintptr_t>(std::addressof(g_fatal_dirty_multi_wait_holder)));
            g_initialized_fatal_dirty_event = true;
        }
        return std::addressof(g_fatal_dirty_multi_wait_holder);
    }

    void OnFatalDirtyEvent() {
        os::ClearSystemEvent(std::addressof(g_fatal_dirty_event));

        u64 flags_0, flags_1;
        if (R_SUCCEEDED(setsysGetFatalDirtyFlags(std::addressof(flags_0), std::addressof(flags_1))) && (flags_0 & 1)) {
            GetFatalConfigImpl().UpdateLanguageCode();
        }
    }

    FatalConfig::FatalConfig() {
        /* Get information from set. */
        settings::system::GetSerialNumber(std::addressof(m_serial_number));
        settings::system::GetFirmwareVersion(std::addressof(m_firmware_version));
        setsysGetQuestFlag(std::addressof(m_quest_flag));
        this->UpdateLanguageCode();

        /* Read information from settings. */
        settings::fwdbg::GetSettingsItemValue(std::addressof(m_transition_to_fatal), sizeof(m_transition_to_fatal), "fatal", "transition_to_fatal");
        settings::fwdbg::GetSettingsItemValue(std::addressof(m_show_extra_info), sizeof(m_show_extra_info), "fatal", "show_extra_info");

        u64 quest_interval_second;
        settings::fwdbg::GetSettingsItemValue(std::addressof(quest_interval_second), sizeof(quest_interval_second), "fatal", "quest_reboot_interval_second");
        m_quest_reboot_interval = TimeSpan::FromSeconds(quest_interval_second);

        /* Atmosphere extension for automatic reboot. */
        u64 auto_reboot_ms;
        if (settings::fwdbg::GetSettingsItemValue(std::addressof(auto_reboot_ms), sizeof(auto_reboot_ms), "atmosphere", "fatal_auto_reboot_interval") == sizeof(auto_reboot_ms)) {
            m_fatal_auto_reboot_interval = TimeSpan::FromMilliSeconds(auto_reboot_ms);
            m_fatal_auto_reboot_enabled  = auto_reboot_ms != 0;
        }

        /* Setup messages. */
        {
            m_error_msg = "Error Code: 2%03d-%04d (0x%x)\n";

            m_error_desc = "An error has occurred.\n\n"
                           "Please press the POWER Button to restart the console normally, or a VOL button\n"
                           "to reboot to a payload (or RCM, if none is present). If you are unable to\n"
                           "restart the console, hold the POWER Button for 12 seconds to turn the console off.\n\n"
                           "If the problem persists, refer to the Nintendo Support Website.\n"
                           "support.nintendo.com/switch/error\n";

            /* If you're running Atmosphere on a quest unit for some reason, talk to me on discord. */
            m_quest_desc = "Please call 1-800-875-1852 for service.\n\n"
                           "Also, please be aware that running Atmosphere on a Quest device is not fully\n"
                           "supported. Perhaps try booting your device without Atmosphere before calling\n"
                           "an official Nintendo service hotline. If you encounter further issues, please\n"
                           "contact SciresM#0524 on Discord, or via some other means.\n";

            /* TODO: Try to load dynamically? */
            /* FsStorage message_storage; */
            /* TODO: if (R_SUCCEEDED(fsOpenDataStorageByDataId(0x010000000000081D, "fatal_msg"))) { ... } */
        }
    }

    const FatalConfig &GetFatalConfig() {
        return GetFatalConfigImpl();
    }

}
