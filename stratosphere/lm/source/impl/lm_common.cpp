#include "lm_common.hpp"

namespace ams::lm::impl {

    u64 PopUleb128(const u8 **buf, const u8 *max_buf) {
        size_t shift = 0;
        u64 value = 0;
        while (*buf <= max_buf) {
            if ((shift + 7) > 0x40) {
                return 0;
            }
            auto byte = **buf;
            value |= (byte & 0x7F) << shift;
            shift += 7;
            (*buf)++;
            if (!(byte & 0x80)) {
                break;
            }
        }
        return value;
    }

}