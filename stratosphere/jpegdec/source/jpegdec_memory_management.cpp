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
#include "jpegdec_memory_management.hpp"

namespace ams::jpegdec {

    namespace {

        /* TODO: Update libjpeg-turbo to include Nintendo's changes (support for work buffer, rather than malloc) */
        constexpr size_t JpegHeapSize = 96_KB;
        alignas(0x10) constinit u8 g_jpeg_heap_memory[JpegHeapSize];

        constinit lmem::HeapHandle g_jpeg_heap_handle;

    }

    void InitializeJpegHeap() {
        g_jpeg_heap_handle = lmem::CreateExpHeap(g_jpeg_heap_memory, sizeof(g_jpeg_heap_memory), lmem::CreateOption_None);
    }

}

extern "C" void *__wrap_jpeg_get_small(void *cinfo, size_t size) {
    AMS_UNUSED(cinfo);
    return ::ams::lmem::AllocateFromExpHeap(::ams::jpegdec::g_jpeg_heap_handle, size);
}

extern "C" void __wrap_jpeg_free_small(void *cinfo, void *ptr, size_t size) {
    AMS_UNUSED(cinfo, size);
    return ::ams::lmem::FreeToExpHeap(::ams::jpegdec::g_jpeg_heap_handle, ptr);
}

extern "C" void *__wrap_jpeg_get_large(void *cinfo, size_t size) {
    AMS_UNUSED(cinfo);
    return ::ams::lmem::AllocateFromExpHeap(::ams::jpegdec::g_jpeg_heap_handle, size);
}

extern "C" void __wrap_jpeg_free_large(void *cinfo, void *ptr, size_t size) {
    AMS_UNUSED(cinfo, size);
    return ::ams::lmem::FreeToExpHeap(::ams::jpegdec::g_jpeg_heap_handle, ptr);
}
