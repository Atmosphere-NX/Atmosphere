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
#include <vapours.hpp>

namespace ams::crypto::impl {

    void BigNum::ImportImpl(Word *out, size_t out_size, const u8 *src, size_t src_size) {
        size_t octet_ofs = src_size;
        size_t word_ofs  = 0;

        /* Parse octets into words. */
        while (word_ofs < out_size && octet_ofs > 0) {
            Word w = 0;
            for (size_t shift = 0; octet_ofs > 0 && shift < BITSIZEOF(Word); shift += BITSIZEOF(u8)) {
                w |= static_cast<Word>(src[--octet_ofs]) << shift;
            }
            out[word_ofs++] = w;
        }

        /* Zero-fill upper words. */
        while (word_ofs < out_size) {
            out[word_ofs++] = 0;
        }
    }

    void BigNum::ExportImpl(u8 *out, size_t out_size, const Word *src, size_t src_size) {
        size_t octet_ofs = out_size;

        /* Parse words into octets. */
        for (size_t word_ofs = 0; word_ofs < src_size && octet_ofs > 0; word_ofs++) {
            const Word w = src[word_ofs];
            for (size_t shift = 0; octet_ofs > 0 && shift < BITSIZEOF(Word); shift += BITSIZEOF(u8)) {
                out[--octet_ofs] = static_cast<u8>(w >> shift);
            }
        }

        /* Zero-clear remaining octets. */
        while (octet_ofs > 0) {
            out[--octet_ofs] = 0;
        }
    }

    size_t BigNum::GetSize() const {
        if (m_num_words == 0) {
            return 0;
        }
        static_assert(sizeof(Word) == 4);

        size_t size = m_num_words * sizeof(Word);
        const Word last = m_words[m_num_words - 1];
        AMS_ASSERT(last != 0);
        if (last >= 0x01000000u) {
            return size - 0;
        } else if (last >= 0x00010000u) {
            return size - 1;
        } else if (last >= 0x00000100u) {
            return size - 2;
        } else {
            return size - 3;
        }
    }

    bool BigNum::Import(const void *src, size_t src_size) {
        AMS_ASSERT((src != nullptr) || (src_size != 0));

        /* Ignore leading zeroes. */
        const u8 *data = static_cast<const u8 *>(src);
        while (src_size > 0 && *data == 0) {
            ++data;
            --src_size;
        }

        /* Ensure we have space for the number. */
        AMS_ASSERT(src_size <= m_max_words * sizeof(Word));
        if (AMS_UNLIKELY(!(src_size <= m_max_words * sizeof(Word)))) {
            return false;
        }

        /* Import. */
        m_num_words = util::AlignUp(src_size, sizeof(Word)) / sizeof(Word);

        ImportImpl(m_words, m_max_words, data, src_size);
        return true;
    }

    void BigNum::Export(void *dst, size_t dst_size) {
        AMS_ASSERT(dst_size >= this->GetSize());
        ExportImpl(static_cast<u8 *>(dst), dst_size, m_words, m_num_words);
    }

    bool BigNum::ExpMod(void *dst, const void *src, size_t size, const BigNum &exp, u32 *work_buf, size_t work_buf_size) const {
        /* Can't exponentiate with or about zero. */
        if (this->IsZero() || exp.IsZero()) {
            return false;
        }
        AMS_ASSERT(size == this->GetSize());

        /* Create an allocator. */
        WordAllocator allocator(work_buf, work_buf_size / sizeof(Word));
        ON_SCOPE_EXIT { ClearMemory(work_buf, allocator.GetMaxUsedSize()); };

        /* Create a BigNum for the signature. */
        BigNum signature;
        auto signature_words = allocator.Allocate(size / sizeof(Word));
        if (!signature_words.IsValid()) {
            return false;
        }

        /* Import data for the signature. */
        signature.ReserveStatic(signature_words.GetBuffer(), signature_words.GetCount());
        if (!signature.Import(src, size)) {
            return false;
        }

        /* Perform the exponentiation. */
        if (!ExpMod(signature.m_words, signature.m_words, exp.m_words, exp.m_num_words, m_words, m_num_words, std::addressof(allocator))) {
            return false;
        }

        /* We succeeded, so export. */
        signature.UpdateCount();
        signature.Export(dst, size);

        return true;
    }

    void BigNum::ClearToZero() {
        std::memset(m_words, 0, m_num_words * sizeof(Word));
    }

    void BigNum::UpdateCount() {
        m_num_words = CountWords(m_words, m_max_words);
    }

}