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
#pragma once
#include <vapours.hpp>

namespace ams::tipc {

    namespace impl {

        template<typename HandleType>
        class DummyMessageBuffer {
            public:
                class MessageHeader {
                    public:
                        MessageHeader() { AMS_ABORT("TODO"); }
                        MessageHeader(const DummyMessageBuffer &) { AMS_ABORT("TODO"); }

                        u16 GetTag() const { AMS_ABORT("TODO"); }
                };
                class SpecialHeader {
                    public:
                        SpecialHeader() { AMS_ABORT("TODO"); }
                        SpecialHeader(const DummyMessageBuffer &, const MessageHeader &) { AMS_ABORT("TODO"); }

                        bool GetHasProcessId() const { AMS_ABORT("TODO"); }
                        s32 GetCopyHandleCount() const { AMS_ABORT("TODO"); }
                };
            public:
                DummyMessageBuffer(u32 *) { AMS_ABORT("TODO"); }
                DummyMessageBuffer(u32 *, size_t) { AMS_ABORT("TODO"); }

                void SetNull() const { AMS_ABORT("TODO"); }

                HandleType GetHandle(s32) const { AMS_ABORT("TODO"); }

                template<typename T>
                const T &GetRaw(s32) const { AMS_ABORT("TODO"); }

                static s32 GetSpecialDataIndex(const MessageHeader &, const SpecialHeader &) { AMS_ABORT("TODO"); }
                static s32 GetRawDataIndex(const MessageHeader &, const SpecialHeader &) { AMS_ABORT("TODO"); }
        };

    }

    #if defined(ATMOSPHERE_OS_HORIZON)

        constexpr inline auto MaximumSessionsPerPort = svc::ArgumentHandleCountMax;

        using NativeHandle = svc::Handle;
        constexpr inline NativeHandle InvalidNativeHandle = svc::InvalidHandle;

        using MessageBuffer = ::ams::svc::ipc::MessageBuffer;

        ALWAYS_INLINE u32 *GetMessageBuffer() { return svc::ipc::GetMessageBuffer(); }

        constexpr ALWAYS_INLINE u32 EncodeNativeHandleForMessageQueue(NativeHandle h) { return static_cast<u32>(h); }
        constexpr ALWAYS_INLINE NativeHandle DecodeNativeHandleForMessageQueue(u32 v) { return static_cast<NativeHandle>(v); }

    #elif defined(ATMOSPHERE_OS_WINDOWS)

        /* TODO */
        constexpr inline auto MaximumSessionsPerPort = 0x40;

        using NativeHandle = void *;
        constexpr inline NativeHandle InvalidNativeHandle = nullptr;

        using MessageBuffer = ::ams::tipc::impl::DummyMessageBuffer<NativeHandle>;

        ALWAYS_INLINE u32 *GetMessageBuffer() { AMS_ABORT("TODO"); }

        ALWAYS_INLINE u32 EncodeNativeHandleForMessageQueue(NativeHandle) { AMS_ABORT("TODO"); }
        ALWAYS_INLINE NativeHandle DecodeNativeHandleForMessageQueue(u32) { AMS_ABORT("TODO"); }

    #elif defined(ATMOSPHERE_OS_LINUX)

        /* TODO */
        constexpr inline auto MaximumSessionsPerPort = 0x40;

        using NativeHandle = s32;
        constexpr inline NativeHandle InvalidNativeHandle = -1;

        using MessageBuffer = ::ams::tipc::impl::DummyMessageBuffer<NativeHandle>;

        ALWAYS_INLINE u32 *GetMessageBuffer() { AMS_ABORT("TODO"); }

        ALWAYS_INLINE u32 EncodeNativeHandleForMessageQueue(NativeHandle) { AMS_ABORT("TODO"); }
        ALWAYS_INLINE NativeHandle DecodeNativeHandleForMessageQueue(u32) { AMS_ABORT("TODO"); }

    #elif defined(ATMOSPHERE_OS_MACOS)

        /* TODO */
        constexpr inline auto MaximumSessionsPerPort = 0x40;

        using NativeHandle = s32;
        constexpr inline NativeHandle InvalidNativeHandle = -1;

        using MessageBuffer = ::ams::tipc::impl::DummyMessageBuffer<NativeHandle>;

        ALWAYS_INLINE u32 *GetMessageBuffer() { AMS_ABORT("TODO"); }

        ALWAYS_INLINE u32 EncodeNativeHandleForMessageQueue(NativeHandle) { AMS_ABORT("TODO"); }
        ALWAYS_INLINE NativeHandle DecodeNativeHandleForMessageQueue(u32) { AMS_ABORT("TODO"); }

    #else
        #error "Unknown OS for tipc platform types."
    #endif

    namespace impl {



    }

}

