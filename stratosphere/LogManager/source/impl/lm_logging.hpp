
#pragma once
#include "../lm_types.hpp"

namespace ams::lm::impl {

    constexpr const char DebugLogDirectory[] = "sdmc:/atmosphere/debug_logs";

    enum LogPacketFlags : u8 {
        LogPacketFlags_Head = BIT(0), /* Head -> a packet list is being sent, with this packet being the initial one. */
        LogPacketFlags_Tail = BIT(1), /* Tail -> this is the final packet of the packet list. */
        LogPacketFlags_LittleEndian = BIT(2),
    };

    struct LogPacketHeader {
        u64 process_id;
        u64 thread_id;
        u8 flags;
        u8 pad;
        u8 severity;
        u8 verbosity;
        u32 payload_size;

        inline constexpr bool IsHead() const {
            return this->flags & LogPacketFlags_Head;
        }

        inline constexpr bool IsTail() const {
            return this->flags & LogPacketFlags_Tail;
        }

    };
    static_assert(sizeof(LogPacketHeader) == 0x18, "LogPacketHeader definition");

    struct LogInfo {
        s64 log_id; /* This is the system tick value when the log was saved. */
        u64 program_id;
    };

    /* Store log packet buffers as a unique_ptr, so that they automatically get disposed after they're used. */

    struct LogPacketBuffer {
        u64 program_id;
        std::unique_ptr<u8[]> buf;
        size_t buf_size;

        LogPacketBuffer() : program_id(0), buf(nullptr), buf_size(0) {}

        LogPacketBuffer(u64 program_id, const void *buf, size_t size) : program_id(program_id), buf(std::make_unique<u8[]>(size)), buf_size(size) {
            if(this->buf != nullptr) {
                std::memcpy(this->buf.get(), buf, size);
            }
        }

        inline const LogPacketHeader *GetHeader() const {
            if(this->buf == nullptr) {
                return nullptr;
            }
            return reinterpret_cast<const LogPacketHeader*>(this->buf.get());
        }

        inline size_t GetPacketSize() const {
            if(this->buf == nullptr) {
                return 0;
            }
            auto header = this->GetHeader();
            return sizeof(LogPacketHeader) + header->payload_size;
        }

        inline bool ValidatePacket() const {
            if(this->buf == nullptr) {
                return false;
            }
            /* Ensure that the buffer size is big enough to properly hold the packet. */
            /* Input buffers might be bigger than the actual packet size. */
            return this->buf_size >= this->GetPacketSize();
        }

        inline bool IsHead() const {
            if(this->buf == nullptr) {
                return false;
            }
            auto header = this->GetHeader();
            return header->IsHead();
        }

        inline bool IsTail() const {
            if(this->buf == nullptr) {
                return false;
            }
            auto header = this->GetHeader();
            return header->IsTail();
        }

        inline u64 GetProgramId() const {
            return this->program_id;
        }

    };

    inline void ClearDebugLogs() {
        fs::DeleteDirectoryRecursively(DebugLogDirectory);
    }

    void SetCanAccessFs(bool can_access);
    Result WriteLogPackets(std::vector<LogPacketBuffer> &packet_list);
    
    LogInfo GetLastLogInfo();

    Handle GetLogEventHandle();

}