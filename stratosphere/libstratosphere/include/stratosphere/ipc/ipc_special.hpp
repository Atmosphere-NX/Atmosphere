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

#include "ipc_out.hpp"

/* Represents an input PID. */
struct PidDescriptorTag{};

struct PidDescriptor : public PidDescriptorTag {
    u64 pid;

    void operator=(u64 &p) {
        pid = p;
    }

    PidDescriptor(u64 p) : pid(p) { }
};

struct IpcHandleTag{};

struct IpcHandle : public IpcHandleTag {
    Handle handle;
};

/* Represents a moved handle. */
struct MovedHandle : public IpcHandle {
    void operator=(const Handle &h) {
        this->handle = h;
    }

    void operator=(const IpcHandle &o) {
        this->handle = o.handle;
    }

    MovedHandle(Handle h) {
        this->handle = h;
    }

    Handle GetValue() const {
        return this->handle;
    }
};

/* Represents a copied handle. */
struct CopiedHandle : public IpcHandle {
    void operator=(const Handle &h) {
        handle = h;
    }

    void operator=(const IpcHandle &o) {
        this->handle = o.handle;
    }

    CopiedHandle(Handle h) {
        this->handle = h;
    }

    Handle GetValue() const {
        return this->handle;
    }
};

template <>
class Out<MovedHandle> : public OutHandleTag {
private:
    MovedHandle *obj;
public:
    Out(IpcHandle *o) : obj(static_cast<MovedHandle *>(o)) { }

    void SetValue(const Handle& h) {
        *obj = h;
    }

    void SetValue(const MovedHandle& o) {
        *obj = o;
    }

    const MovedHandle& GetValue() {
        return *obj;
    }

    MovedHandle* GetPointer() {
        return obj;
    }

    Handle* GetHandlePointer() {
        return &obj->handle;
    }

    /* Convenience operators. */
    MovedHandle& operator*() {
        return *obj;
    }

    MovedHandle* operator->() {
        return obj;
    }
};

template <>
class Out<CopiedHandle> : public OutHandleTag {
private:
    CopiedHandle *obj;
public:
    Out(IpcHandle *o) : obj(static_cast<CopiedHandle *>(o)) { }

    void SetValue(const Handle& h) {
        *obj = h;
    }

    void SetValue(const CopiedHandle& o) {
        *obj = o;
    }

    const CopiedHandle& GetValue() {
        return *obj;
    }

    CopiedHandle* GetPointer() {
        return obj;
    }

    Handle* GetHandlePointer() {
        return &obj->handle;
    }

    /* Convenience operators. */
    CopiedHandle& operator*() {
        return *obj;
    }

    CopiedHandle* operator->() {
        return obj;
    }
};