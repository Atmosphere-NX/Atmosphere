/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <type_traits>

enum class IpcBufferType {
    InBuffer,
    OutBuffer,
    InPointer,
    OutPointer,
};

/* Base for In/Out Buffers. */
struct IpcBufferBase {};

struct InOutBufferBase : public IpcBufferBase {};

/* Represents an A descriptor. */
struct InBufferBase : public InOutBufferBase {};

template <typename T, BufferType e_t = BufferType_Normal>
struct InBuffer : public InBufferBase {
    T *buffer;
    size_t num_elements;
    BufferType type;
    static const BufferType expected_type = e_t;

    /* Convenience. */
    T& operator[](size_t i) const {
        return buffer[i];
    }

    InBuffer(void *b, size_t n, BufferType t) : buffer((T *)b), num_elements(n/sizeof(T)), type(t) { }
};

/* Represents a B descriptor. */
struct OutBufferBase : public InOutBufferBase {};

template <typename T, BufferType e_t = BufferType_Normal>
struct OutBuffer : OutBufferBase {
    T *buffer;
    size_t num_elements;
    BufferType type;
    static const BufferType expected_type = e_t;

    /* Convenience. */
    T& operator[](size_t i) const {
        return buffer[i];
    }

    OutBuffer(void *b, size_t n, BufferType t) : buffer((T *)b), num_elements(n/sizeof(T)), type(t) { }
};

/* Represents an X descriptor. */
struct InPointerBase : public IpcBufferBase {};

template <typename T>
struct InPointer : public InPointerBase {
    T *pointer;
    size_t num_elements;

    /* Convenience. */
    T& operator[](size_t i) const {
        return pointer[i];
    }

    InPointer(void *p, size_t n) : pointer((T *)p), num_elements(n/sizeof(T)) { }
};

/* Represents a C descriptor. */
struct OutPointerWithServerSizeBase : public IpcBufferBase {};

template <typename T, size_t N>
struct OutPointerWithServerSize : public OutPointerWithServerSizeBase {
    T *pointer;
    static const size_t num_elements = N;
    static const size_t element_size = sizeof(T);

    /* Convenience. */
    T& operator[](size_t i) const {
        return pointer[i];
    }

    OutPointerWithServerSize(void *p) : pointer((T *)p) { }
    OutPointerWithServerSize(void *p, size_t n) : pointer((T *)p) { }
};

struct OutPointerWithClientSizeBase : public IpcBufferBase {};

/* Represents a C descriptor with size in raw data. */
template <typename T>
struct OutPointerWithClientSize : public OutPointerWithClientSizeBase {
    T *pointer;
    size_t num_elements;

    /* Convenience. */
    T& operator[](size_t i) const {
        return pointer[i];
    }

    OutPointerWithClientSize(void *p, size_t n) : pointer((T *)p), num_elements(n/sizeof(T)) { }
};
