/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include "impl/os_waitable_object_list.hpp"

namespace ams::os {

    MessageQueue::MessageQueue(std::unique_ptr<uintptr_t[]> buf, size_t c): buffer(std::move(buf)), capacity(c), count(0), offset(0) {
        new (GetPointer(this->waitlist_not_empty)) impl::WaitableObjectList();
        new (GetPointer(this->waitlist_not_full))  impl::WaitableObjectList();
    }

    MessageQueue::~MessageQueue() {
        GetReference(this->waitlist_not_empty).~WaitableObjectList();
        GetReference(this->waitlist_not_full).~WaitableObjectList();
    }

    void MessageQueue::SendInternal(uintptr_t data) {
        /* Ensure we don't corrupt the queue, but this should never happen. */
        AMS_ABORT_UNLESS(this->count < this->capacity);

        /* Write data to tail of queue. */
        this->buffer[(this->count++ + this->offset) % this->capacity] = data;
    }

    void MessageQueue::SendNextInternal(uintptr_t data) {
        /* Ensure we don't corrupt the queue, but this should never happen. */
        AMS_ABORT_UNLESS(this->count < this->capacity);

        /* Write data to head of queue. */
        this->offset = (this->offset + this->capacity - 1) % this->capacity;
        this->buffer[this->offset] = data;
        this->count++;
    }

    uintptr_t MessageQueue::ReceiveInternal() {
        /* Ensure we don't corrupt the queue, but this should never happen. */
        AMS_ABORT_UNLESS(this->count > 0);

        uintptr_t data = this->buffer[this->offset];
        this->offset = (this->offset + 1) % this->capacity;
        this->count--;
        return data;
    }

    inline uintptr_t MessageQueue::PeekInternal() {
        /* Ensure we don't corrupt the queue, but this should never happen. */
        AMS_ABORT_UNLESS(this->count > 0);

        return this->buffer[this->offset];
    }

    void MessageQueue::Send(uintptr_t data) {
        /* Acquire mutex, wait sendable. */
        std::scoped_lock lock(this->queue_lock);

        while (this->IsFull()) {
            this->cv_not_full.Wait(&this->queue_lock);
        }

        /* Send, signal. */
        this->SendInternal(data);
        this->cv_not_empty.Broadcast();
        GetReference(this->waitlist_not_empty).SignalAllThreads();
    }

    bool MessageQueue::TrySend(uintptr_t data) {
        std::scoped_lock lock(this->queue_lock);
        if (this->IsFull()) {
            return false;
        }

        /* Send, signal. */
        this->SendInternal(data);
        this->cv_not_empty.Broadcast();
        GetReference(this->waitlist_not_empty).SignalAllThreads();
        return true;
    }

    bool MessageQueue::TimedSend(uintptr_t data, u64 timeout) {
        std::scoped_lock lock(this->queue_lock);
        TimeoutHelper timeout_helper(timeout);

        while (this->IsFull()) {
            if (timeout_helper.TimedOut()) {
                return false;
            }

            this->cv_not_full.TimedWait(&this->queue_lock, timeout_helper.NsUntilTimeout());
        }

        /* Send, signal. */
        this->SendInternal(data);
        this->cv_not_empty.Broadcast();
        GetReference(this->waitlist_not_empty).SignalAllThreads();
        return true;
    }

    void MessageQueue::SendNext(uintptr_t data) {
        /* Acquire mutex, wait sendable. */
        std::scoped_lock lock(this->queue_lock);

        while (this->IsFull()) {
            this->cv_not_full.Wait(&this->queue_lock);
        }

        /* Send, signal. */
        this->SendNextInternal(data);
        this->cv_not_empty.Broadcast();
        GetReference(this->waitlist_not_empty).SignalAllThreads();
    }

    bool MessageQueue::TrySendNext(uintptr_t data) {
        std::scoped_lock lock(this->queue_lock);
        if (this->IsFull()) {
            return false;
        }

        /* Send, signal. */
        this->SendNextInternal(data);
        this->cv_not_empty.Broadcast();
        GetReference(this->waitlist_not_empty).SignalAllThreads();
        return true;
    }

    bool MessageQueue::TimedSendNext(uintptr_t data, u64 timeout) {
        std::scoped_lock lock(this->queue_lock);
        TimeoutHelper timeout_helper(timeout);

        while (this->IsFull()) {
            if (timeout_helper.TimedOut()) {
                return false;
            }

            this->cv_not_full.TimedWait(&this->queue_lock, timeout_helper.NsUntilTimeout());
        }

        /* Send, signal. */
        this->SendNextInternal(data);
        this->cv_not_empty.Broadcast();
        GetReference(this->waitlist_not_empty).SignalAllThreads();
        return true;
    }

    void MessageQueue::Receive(uintptr_t *out) {
        /* Acquire mutex, wait receivable. */
        std::scoped_lock lock(this->queue_lock);

        while (this->IsEmpty()) {
            this->cv_not_empty.Wait(&this->queue_lock);
        }

        /* Receive, signal. */
        *out = this->ReceiveInternal();
        this->cv_not_full.Broadcast();
        GetReference(this->waitlist_not_full).SignalAllThreads();
    }

    bool MessageQueue::TryReceive(uintptr_t *out) {
        /* Acquire mutex, wait receivable. */
        std::scoped_lock lock(this->queue_lock);

        if (this->IsEmpty()) {
            return false;
        }

        /* Receive, signal. */
        *out = this->ReceiveInternal();
        this->cv_not_full.Broadcast();
        GetReference(this->waitlist_not_full).SignalAllThreads();
        return true;
    }

    bool MessageQueue::TimedReceive(uintptr_t *out, u64 timeout) {
        std::scoped_lock lock(this->queue_lock);
        TimeoutHelper timeout_helper(timeout);

        while (this->IsEmpty()) {
            if (timeout_helper.TimedOut()) {
                return false;
            }

            this->cv_not_empty.TimedWait(&this->queue_lock, timeout_helper.NsUntilTimeout());
        }

        /* Receive, signal. */
        *out = this->ReceiveInternal();
        this->cv_not_full.Broadcast();
        GetReference(this->waitlist_not_full).SignalAllThreads();
        return true;
    }

    void MessageQueue::Peek(uintptr_t *out) {
        /* Acquire mutex, wait receivable. */
        std::scoped_lock lock(this->queue_lock);

        while (this->IsEmpty()) {
            this->cv_not_empty.Wait(&this->queue_lock);
        }

        /* Peek. */
        *out = this->PeekInternal();
    }

    bool MessageQueue::TryPeek(uintptr_t *out) {
        /* Acquire mutex, wait receivable. */
        std::scoped_lock lock(this->queue_lock);

        if (this->IsEmpty()) {
            return false;
        }

        /* Peek. */
        *out = this->PeekInternal();
        return true;
    }

    bool MessageQueue::TimedPeek(uintptr_t *out, u64 timeout) {
        std::scoped_lock lock(this->queue_lock);
        TimeoutHelper timeout_helper(timeout);

        while (this->IsEmpty()) {
            if (timeout_helper.TimedOut()) {
                return false;
            }

            this->cv_not_empty.TimedWait(&this->queue_lock, timeout_helper.NsUntilTimeout());
        }

        /* Peek. */
        *out = this->PeekInternal();
        return true;
    }

}
