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
#include <mesosphere.hpp>

void operator delete (void *deleted) throw() {
    MESOSPHERE_PANIC("operator delete(void *) was called: %p", deleted);
}

void operator delete (void *deleted, size_t size) throw() {
    MESOSPHERE_PANIC("operator delete(void *, size_t) was called: %p %zu", deleted, size);
}

void operator delete (void *deleted, size_t size, std::align_val_t align) throw() {
    MESOSPHERE_PANIC("operator delete(void *, size_t, std::align_val_t) was called: %p %zu, %zu", deleted, size, static_cast<size_t>(align));
}

extern "C" void abort() {
    MESOSPHERE_PANIC("abort() was called");
}
