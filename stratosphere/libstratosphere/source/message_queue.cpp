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

#include <mutex>
#include <switch.h>
#include <stratosphere.hpp>


void HosMessageQueue::Send(uintptr_t data) {
    /* Acquire mutex, wait sendable. */
    std::scoped_lock<HosMutex> lock(this->queue_lock);

    while (this->IsFull()) {
        this->cv_not_full.Wait(&this->queue_lock);
    }

    /* Send, signal. */
    this->SendInternal(data);
    this->cv_not_empty.WakeAll();
}

bool HosMessageQueue::TrySend(uintptr_t data) {
    std::scoped_lock<HosMutex> lock(this->queue_lock);
    if (this->IsFull()) {
        return false;
    }

    /* Send, signal. */
    this->SendInternal(data);
    this->cv_not_empty.WakeAll();
    return true;
}

bool HosMessageQueue::TimedSend(uintptr_t data, u64 timeout) {
    std::scoped_lock<HosMutex> lock(this->queue_lock);
    TimeoutHelper timeout_helper(timeout);

    while (this->IsFull()) {
        if (timeout_helper.TimedOut()) {
            return false;
        }

        this->cv_not_full.TimedWait(timeout, &this->queue_lock);
    }

    /* Send, signal. */
    this->SendInternal(data);
    this->cv_not_empty.WakeAll();
    return true;
}

void HosMessageQueue::SendNext(uintptr_t data) {
    /* Acquire mutex, wait sendable. */
    std::scoped_lock<HosMutex> lock(this->queue_lock);

    while (this->IsFull()) {
        this->cv_not_full.Wait(&this->queue_lock);
    }

    /* Send, signal. */
    this->SendNextInternal(data);
    this->cv_not_empty.WakeAll();
}

bool HosMessageQueue::TrySendNext(uintptr_t data) {
    std::scoped_lock<HosMutex> lock(this->queue_lock);
    if (this->IsFull()) {
        return false;
    }

    /* Send, signal. */
    this->SendNextInternal(data);
    this->cv_not_empty.WakeAll();
    return true;
}

bool HosMessageQueue::TimedSendNext(uintptr_t data, u64 timeout) {
    std::scoped_lock<HosMutex> lock(this->queue_lock);
    TimeoutHelper timeout_helper(timeout);

    while (this->IsFull()) {
        if (timeout_helper.TimedOut()) {
            return false;
        }

        this->cv_not_full.TimedWait(timeout, &this->queue_lock);
    }

    /* Send, signal. */
    this->SendNextInternal(data);
    this->cv_not_empty.WakeAll();
    return true;
}

void HosMessageQueue::Receive(uintptr_t *out) {
    /* Acquire mutex, wait receivable. */
    std::scoped_lock<HosMutex> lock(this->queue_lock);

    while (this->IsEmpty()) {
        this->cv_not_empty.Wait(&this->queue_lock);
    }

    /* Receive, signal. */
    *out = this->ReceiveInternal();
    this->cv_not_full.WakeAll();
}
bool HosMessageQueue::TryReceive(uintptr_t *out) {
    /* Acquire mutex, wait receivable. */
    std::scoped_lock<HosMutex> lock(this->queue_lock);

    if (this->IsEmpty()) {
        return false;
    }

    /* Receive, signal. */
    *out = this->ReceiveInternal();
    this->cv_not_full.WakeAll();
    return true;
}

bool HosMessageQueue::TimedReceive(uintptr_t *out, u64 timeout) {
    std::scoped_lock<HosMutex> lock(this->queue_lock);
    TimeoutHelper timeout_helper(timeout);

    while (this->IsEmpty()) {
        if (timeout_helper.TimedOut()) {
            return false;
        }

        this->cv_not_empty.TimedWait(timeout, &this->queue_lock);
    }

    /* Receive, signal. */
    *out = this->ReceiveInternal();
    this->cv_not_full.WakeAll();
    return true;
}

void HosMessageQueue::Peek(uintptr_t *out) {
    /* Acquire mutex, wait receivable. */
    std::scoped_lock<HosMutex> lock(this->queue_lock);

    while (this->IsEmpty()) {
        this->cv_not_empty.Wait(&this->queue_lock);
    }

    /* Peek. */
    *out = this->PeekInternal();
}

bool HosMessageQueue::TryPeek(uintptr_t *out) {
    /* Acquire mutex, wait receivable. */
    std::scoped_lock<HosMutex> lock(this->queue_lock);

    if (this->IsEmpty()) {
        return false;
    }

    /* Peek. */
    *out = this->PeekInternal();
    return true;
}

bool HosMessageQueue::TimedPeek(uintptr_t *out, u64 timeout) {
    std::scoped_lock<HosMutex> lock(this->queue_lock);
    TimeoutHelper timeout_helper(timeout);

    while (this->IsEmpty()) {
        if (timeout_helper.TimedOut()) {
            return false;
        }

        this->cv_not_empty.TimedWait(timeout, &this->queue_lock);
    }

    /* Peek. */
    *out = this->PeekInternal();
    return true;
}

void HosMessageQueue::SendInternal(uintptr_t data) {
    /* Ensure we don't corrupt the queue, but this should never happen. */
    if (this->count >= this->capacity) {
        std::abort();
    }

    /* Write data to tail of queue. */
    this->buffer[(this->count++ + this->offset) % this->capacity] = data;
}

void HosMessageQueue::SendNextInternal(uintptr_t data) {
    /* Ensure we don't corrupt the queue, but this should never happen. */
    if (this->count >= this->capacity) {
        std::abort();
    }

    /* Write data to head of queue. */
    this->offset = (this->offset + this->capacity - 1) % this->capacity;
    this->buffer[this->offset] = data;
    this->count++;
}

uintptr_t HosMessageQueue::ReceiveInternal() {
    /* Ensure we don't corrupt the queue, but this should never happen. */
    if (this->count == 0) {
        std::abort();
    }

    uintptr_t data = this->buffer[this->offset];
    this->offset = (this->offset + 1) % this->capacity;
    this->count--;
    return data;
}

uintptr_t HosMessageQueue::PeekInternal() {
    /* Ensure we don't corrupt the queue, but this should never happen. */
    if (this->count == 0) {
        std::abort();
    }

    return this->buffer[this->offset];
}