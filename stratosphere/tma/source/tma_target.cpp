/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>

#include "tma_conn_connection.hpp"
#include "tma_conn_usb_connection.hpp"

#include "tma_service_manager.hpp"
#include "tma_power_manager.hpp"

#include "tma_target.hpp"

#include "test/atmosphere_test_service.hpp"
#include "settings/settings_service.hpp"

struct TmaTargetConfig {
    char configuration_id1[0x80];
    char serial_number[0x80];
};

static TmaConnection *g_active_connection = nullptr;
static TmaServiceManager *g_service_manager = nullptr;
static HosMutex g_connection_event_mutex;
static bool g_has_woken_up = false;
static bool g_connected_before_sleep = false;
static bool g_signal_on_disconnect = false;

static TmaUsbConnection *g_usb_connection = nullptr;

static TmaTargetConfig g_target_config = {
    "Unknown",
    "SerialNumber",
};

static void RefreshTargetConfig() {
    setsysInitialize();
    
    /* TODO: setsysGetConfigurationId1(&g_target_config.configuration_id1); */
    
    g_target_config.serial_number[0] = 0;
    setsysGetSerialNumber(g_target_config.serial_number);
    
    setsysExit();
}

static void InitializeServices() {
    g_service_manager->Initialize();
}

static void FinalizeServices() {
    g_service_manager->Finalize();
}

static void SetActiveConnection(TmaConnection *connection) {
    if (g_active_connection != connection) {
        if (g_active_connection != nullptr) {
            FinalizeServices();
            g_service_manager->SetConnection(nullptr);
            g_active_connection->Disconnect();
            g_active_connection = nullptr;
        }
        
        if (connection != nullptr) {
            g_active_connection = connection;
            InitializeServices();
            g_service_manager->SetConnection(g_active_connection);
            g_active_connection->SetServiceManager(g_service_manager);
        }
    }
}

static void OnConnectionEvent(void *arg, ConnectionEvent evt) {
    std::scoped_lock<HosMutex> lk(g_connection_event_mutex);
    
    switch (evt) {
        case ConnectionEvent::Connected:
            {
                bool has_active_connection = false;
                g_has_woken_up = false;
                
                if (arg == g_usb_connection) {
                    SetActiveConnection(g_usb_connection);
                    has_active_connection = true;
                }
                
                if (has_active_connection) {
                    /* TODO: Signal connected */
                }
            }
            break;
        case ConnectionEvent::Disconnected:
            if (g_signal_on_disconnect) {
                /* TODO: Signal disconnected */
            }
            break;
        default:
            std::abort();
            break;
    }
}

static void Wake() {
    if (g_service_manager->GetAsleep()) {
        g_has_woken_up = true;
        
        /* N checks what kind of connection to use here. For now, we only use USB. */
        TmaUsbConnection::InitializeComms();
        g_usb_connection = new TmaUsbConnection();
        g_usb_connection->SetConnectionEventCallback(OnConnectionEvent, g_usb_connection);
        g_usb_connection->Initialize();
        g_active_connection = g_usb_connection;
        
        g_service_manager->Wake(g_active_connection);
    }
}

static void Sleep() {
    if (!g_service_manager->GetAsleep()) {
        if (g_active_connection->IsConnected()) {
            g_connected_before_sleep = true;
            
            /* TODO: Send a packet saying we're going to sleep. */
        } else {
            g_connected_before_sleep = false;
        }
        
        g_service_manager->Sleep();
        g_service_manager->SetConnection(nullptr);
        g_active_connection->Disconnect();
        g_active_connection = nullptr;
        g_service_manager->CancelTasks();
        
        if (g_usb_connection != nullptr) {
            g_usb_connection->Finalize();
            delete g_usb_connection;
            g_usb_connection = nullptr;
            TmaUsbConnection::FinalizeComms();
        }
    }
}

static void OnPowerManagementEvent(PscPmState state, u32 flags) {
    switch (state) {
        case PscPmState_Awake:
            {
                Wake();
            }
            break;
        case PscPmState_ReadyAwaken:
            {
                if (g_service_manager->GetAsleep()) {
                    Wake();
                    if (g_connected_before_sleep)
                    {
                        /* Try to restore a connection. */
                        bool connected = g_service_manager->GetConnected();
                        
                        /* N uses a seven-second timeout, here. */
                        TimeoutHelper timeout_helper(7000000000ULL);
                        while (!connected && !timeout_helper.TimedOut()) {
                            connected = g_service_manager->GetConnected();
                            if (!connected) {
                                /* Sleep for 1ms. */
                                svcSleepThread(1000000ULL);
                            }
                        }
                        
                        if (!connected) {
                            /* TODO: Signal disconnected */
                        }
                    }
                }
            }
            break;
        case PscPmState_ReadySleep:
            {
                Sleep();
            }
            break;
        default:
            /* Don't handle ReadySleepCritical/ReadyAwakenCritical/ReadyShutdown */
            break;
    }
}

void TmaTarget::Initialize() {
    /* Get current thread priority. */
    u32 cur_prio;
    if (R_FAILED(svcGetThreadPriority(&cur_prio, CUR_THREAD_HANDLE))) {
        std::abort();
    }
    
    g_active_connection = nullptr;
    g_service_manager = new TmaServiceManager();
    /* TODO: Make this better. */
    g_service_manager->AddService(new AtmosphereTestService(g_service_manager));
    g_service_manager->AddService(new SettingsService(g_service_manager));
    
    RefreshTargetConfig();
    
    /* N checks what kind of connection to use here. For now, we only use USB. */
    TmaUsbConnection::InitializeComms();
    g_usb_connection = new TmaUsbConnection();
    g_usb_connection->SetConnectionEventCallback(OnConnectionEvent, g_usb_connection);
    g_usb_connection->Initialize();
    SetActiveConnection(g_usb_connection);
    
    /* TODO: Initialize connection events */
    
    /* TODO: Initialize IPC services */
    
    TmaPowerManager::Initialize(OnPowerManagementEvent);
}

void TmaTarget::Finalize() {
    /* TODO: Is implementing this actually worthwhile? It will never be called in practice... */
}
