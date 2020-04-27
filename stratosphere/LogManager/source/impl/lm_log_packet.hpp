
#pragma once
#include <stratosphere.hpp>
#include "../lm_types.hpp"

namespace ams::lm::impl {

    enum class LogDataChunkKey : u8 {
        SessionBegin = 0,
        SessionEnd = 1,
        TextLog = 2,
        LineNumber = 3,
        FileName = 4,
        FunctionName = 5,
        ModuleName = 6,
        ThreadName = 7,
        LogPacketDropCount = 8,
        UserSystemClock = 9,
        ProcessName = 10,
    };

    enum class LogSeverity : u8 {
        Trace = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Fatal = 4,
    };

    enum LogPacketFlags : u8 {
        LogPacketFlags_Head = BIT(0),
        LogPacketFlags_Tail = BIT(1),
    };

    struct LogPacketHeader {
        u64 process_id;
        u64 thread_id;
        u8 flags;
        u8 pad;
        u8 severity;
        u8 verbosity;
        u32 payload_size;

        inline constexpr bool IsHead() {
            /* Head -> a packet list is being sent, with this packet being the initial one. */
            return this->flags & LogPacketFlags_Head;
        }

        inline constexpr bool IsTail() {
            /* Tail -> this is the final packet of the packet list. */
            return this->flags & LogPacketFlags_Tail;
        }

    } PACKED;
    static_assert(sizeof(LogPacketHeader) == 0x18, "LogPacketHeader definition");

    /* Log data chunk base type. */

    struct LogDataChunkTypeHeader {
        u8 key;
        u8 chunk_length;
    } PACKED;

    template<typename T>
    struct LogDataChunkType {
        LogDataChunkTypeHeader header;
        T value;

        inline constexpr bool IsEmpty() {
            /* If it's not present, its length will not be set. */
            return this->header.chunk_length == 0;
        }

    } PACKED;

    /* Since the length is stored as a single byte, the string will never be larger than 0xFF */
    struct LogDataChunkStringType : public LogDataChunkType<char[UINT8_MAX + 1]> {};

    /* Actual chunk base types. */

    struct LogDataChunkSessionBeginType : public LogDataChunkType<bool> {};
    struct LogDataChunkSessionEndType : public LogDataChunkType<bool> {};
    struct LogDataChunkTextLogType : public LogDataChunkStringType {};
    struct LogDataChunkLineNumberType : public LogDataChunkType<u32> {};
    struct LogDataChunkFileNameType : public LogDataChunkStringType {};
    struct LogDataChunkFunctionNameType : public LogDataChunkStringType {};
    struct LogDataChunkModuleNameType : public LogDataChunkStringType {};
    struct LogDataChunkThreadNameType : public LogDataChunkStringType {};
    struct LogDataChunkLogPacketDropCountLogType : public LogDataChunkType<u64> {};
    struct LogDataChunkUserSystemClockType : public LogDataChunkType<u64> {};
    struct LogDataChunkProcessNameType : public LogDataChunkStringType {};

    /* One of each chunk type. */

    struct LogPacketPayload {
        LogDataChunkSessionBeginType session_begin;
        LogDataChunkSessionEndType session_end;
        LogDataChunkTextLogType text_log;
        LogDataChunkLineNumberType line_number;
        LogDataChunkFileNameType file_name;
        LogDataChunkFunctionNameType function_name;
        LogDataChunkModuleNameType module_name;
        LogDataChunkThreadNameType thread_name;
        LogDataChunkLogPacketDropCountLogType log_packet_drop_count;
        LogDataChunkUserSystemClockType user_system_clock;
        LogDataChunkProcessNameType process_name;
    };

    struct LogPacket {
        LogPacketHeader header;
        LogPacketPayload payload;
    };

    template<typename C>
    inline C ParseStringChunkType(const u8 *chunk_buf) {
        static_assert(std::is_base_of_v<LogDataChunkStringType, C>, "Invalid type");

        auto chunk_header = *reinterpret_cast<const LogDataChunkTypeHeader*>(chunk_buf);
        auto type = *reinterpret_cast<const C*>(chunk_buf);
        auto chunk_str_buf = reinterpret_cast<const char*>(chunk_buf + sizeof(LogDataChunkTypeHeader));

        /* Zero the string and copy it from the log buffer. */
        /* This chunk type can't be directly read like the rest, since the string size isn't fixed like with the other types. */
        __builtin_memset(type.value, 0, sizeof(type.value));
        strncpy(type.value, chunk_str_buf, chunk_header.chunk_length);
        return type;
    }

    inline LogPacket ParseLogPacket(const void *buf, size_t size) {
        LogPacket packet = {};
        auto buf8 = reinterpret_cast<const u8*>(buf);
        packet.header = *reinterpret_cast<const LogPacketHeader*>(buf);
        auto payload_buf = buf8 + sizeof(LogPacketHeader);
        size_t offset = 0;
        while(offset < packet.header.payload_size) {
            /* Each chunk data consists on: u8 id, u8 length, u8 data[length]; */
            auto chunk_buf = payload_buf + offset;
            auto chunk_header = *reinterpret_cast<const LogDataChunkTypeHeader*>(chunk_buf);
            /* Parse the chunk depending on the type (strings require special parsing) */
            switch(static_cast<LogDataChunkKey>(chunk_header.key)) {
                case LogDataChunkKey::SessionBegin:
                    packet.payload.session_begin = *reinterpret_cast<const LogDataChunkSessionBeginType*>(chunk_buf);
                    break;
                case LogDataChunkKey::SessionEnd:
                    packet.payload.session_end = *reinterpret_cast<const LogDataChunkSessionEndType*>(chunk_buf);
                    break;
                case LogDataChunkKey::TextLog:
                    packet.payload.text_log = ParseStringChunkType<LogDataChunkTextLogType>(chunk_buf);
                    break;
                case LogDataChunkKey::LineNumber:
                    packet.payload.line_number = *reinterpret_cast<const LogDataChunkLineNumberType*>(chunk_buf);
                    break;
                case LogDataChunkKey::FileName:
                    packet.payload.file_name = ParseStringChunkType<LogDataChunkFileNameType>(chunk_buf);
                    break;
                case LogDataChunkKey::FunctionName:
                    packet.payload.function_name = ParseStringChunkType<LogDataChunkFunctionNameType>(chunk_buf);
                    break;
                case LogDataChunkKey::ModuleName:
                    packet.payload.module_name = ParseStringChunkType<LogDataChunkModuleNameType>(chunk_buf);
                    break;
                case LogDataChunkKey::ThreadName:
                    packet.payload.thread_name = ParseStringChunkType<LogDataChunkThreadNameType>(chunk_buf);
                    break;
                case LogDataChunkKey::LogPacketDropCount:
                    packet.payload.log_packet_drop_count = *reinterpret_cast<const LogDataChunkLogPacketDropCountLogType*>(chunk_buf);
                    break;
                case LogDataChunkKey::UserSystemClock:
                    packet.payload.user_system_clock = *reinterpret_cast<const LogDataChunkUserSystemClockType*>(chunk_buf);
                    break;
                case LogDataChunkKey::ProcessName:
                    packet.payload.process_name = ParseStringChunkType<LogDataChunkProcessNameType>(chunk_buf);
                    break;
            }
            offset += sizeof(LogDataChunkTypeHeader);
            offset += chunk_header.chunk_length;
        }
        return packet;
    }

    void SetCanAccessFs(bool can_access);
    void WriteLogPackets(const char *log_path, std::vector<LogPacket> &packet_list, u64 program_id, LogDestination destination);

}