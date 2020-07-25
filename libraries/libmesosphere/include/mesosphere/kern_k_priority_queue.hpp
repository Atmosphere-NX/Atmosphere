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
#pragma once
#include <mesosphere/kern_common.hpp>

namespace ams::kern {

    template<typename T>
    concept KPriorityQueueAffinityMask = !std::is_reference<T>::value && requires (T &t) {
        { t.GetAffinityMask() } -> std::convertible_to<u64>;
        { t.SetAffinityMask(std::declval<u64>()) };

        { t.GetAffinity(std::declval<int32_t>()) } -> std::same_as<bool>;
        { t.SetAffinity(std::declval<int32_t>(), std::declval<bool>()) };
        { t.SetAll() };
    };

    template<typename T>
    concept KPriorityQueueMember = !std::is_reference<T>::value && requires (T &t) {
        { typename T::QueueEntry() };
        { (typename T::QueueEntry()).Initialize() };
        { (typename T::QueueEntry()).SetPrev(std::addressof(t)) };
        { (typename T::QueueEntry()).SetNext(std::addressof(t)) };
        { (typename T::QueueEntry()).GetNext() }             -> std::same_as<T*>;
        { (typename T::QueueEntry()).GetPrev() }             -> std::same_as<T*>;
        { t.GetPriorityQueueEntry(std::declval<s32>()) } -> std::same_as<typename T::QueueEntry &>;

        { t.GetAffinityMask() };
        { typename std::remove_cvref<decltype(t.GetAffinityMask())>::type() } -> KPriorityQueueAffinityMask;

        { t.GetActiveCore() }   -> std::convertible_to<s32>;
        { t.GetPriority() }     -> std::convertible_to<s32>;
    };

    template<typename Member, size_t _NumCores, int LowestPriority, int HighestPriority> requires KPriorityQueueMember<Member>
    class KPriorityQueue {
        public:
            using AffinityMaskType = typename std::remove_cv<typename std::remove_reference<decltype(std::declval<Member>().GetAffinityMask())>::type>::type;

            static_assert(LowestPriority  >= 0);
            static_assert(HighestPriority >= 0);
            static_assert(LowestPriority  >= HighestPriority);
            static constexpr size_t NumPriority = LowestPriority - HighestPriority + 1;
            static constexpr size_t NumCores    = _NumCores;

            static constexpr ALWAYS_INLINE bool IsValidCore(s32 core) {
                return 0 <= core && core < static_cast<s32>(NumCores);
            }

            static constexpr ALWAYS_INLINE bool IsValidPriority(s32 priority) {
                return HighestPriority <= priority && priority <= LowestPriority + 1;
            }
        private:
            using Entry = typename Member::QueueEntry;
        public:
            class KPerCoreQueue {
                private:
                    Entry root[NumCores];
                public:
                    constexpr ALWAYS_INLINE KPerCoreQueue() : root() {
                        for (size_t i = 0; i < NumCores; i++) {
                            this->root[i].Initialize();
                        }
                    }

                    constexpr ALWAYS_INLINE bool PushBack(s32 core, Member *member) {
                        /* Get the entry associated with the member. */
                        Entry &member_entry = member->GetPriorityQueueEntry(core);

                        /* Get the entry associated with the end of the queue. */
                        Member *tail       = this->root[core].GetPrev();
                        Entry  &tail_entry = (tail != nullptr) ? tail->GetPriorityQueueEntry(core) : this->root[core];

                        /* Link the entries. */
                        member_entry.SetPrev(tail);
                        member_entry.SetNext(nullptr);
                        tail_entry.SetNext(member);
                        this->root[core].SetPrev(member);

                        return (tail == nullptr);
                    }

                    constexpr ALWAYS_INLINE bool PushFront(s32 core, Member *member) {
                        /* Get the entry associated with the member. */
                        Entry &member_entry = member->GetPriorityQueueEntry(core);

                        /* Get the entry associated with the front of the queue. */
                        Member *head       = this->root[core].GetNext();
                        Entry  &head_entry = (head != nullptr) ? head->GetPriorityQueueEntry(core) : this->root[core];

                        /* Link the entries. */
                        member_entry.SetPrev(nullptr);
                        member_entry.SetNext(head);
                        head_entry.SetPrev(member);
                        this->root[core].SetNext(member);

                        return (head == nullptr);
                    }

                    constexpr ALWAYS_INLINE bool Remove(s32 core, Member *member) {
                        /* Get the entry associated with the member. */
                        Entry &member_entry = member->GetPriorityQueueEntry(core);

                        /* Get the entries associated with next and prev. */
                        Member *prev = member_entry.GetPrev();
                        Member *next = member_entry.GetNext();
                        Entry  &prev_entry = (prev != nullptr) ? prev->GetPriorityQueueEntry(core) : this->root[core];
                        Entry  &next_entry = (next != nullptr) ? next->GetPriorityQueueEntry(core) : this->root[core];

                        /* Unlink. */
                        prev_entry.SetNext(next);
                        next_entry.SetPrev(prev);

                        return (this->GetFront(core) == nullptr);
                    }

                    constexpr ALWAYS_INLINE Member *GetFront(s32 core) const {
                        return this->root[core].GetNext();
                    }
            };

            class KPriorityQueueImpl {
                private:
                    KPerCoreQueue queues[NumPriority];
                    util::BitSet64<NumPriority> available_priorities[NumCores];
                public:
                    constexpr ALWAYS_INLINE KPriorityQueueImpl() : queues(), available_priorities() { /* ... */ }

                    constexpr ALWAYS_INLINE void PushBack(s32 priority, s32 core, Member *member) {
                        MESOSPHERE_ASSERT(IsValidCore(core));
                        MESOSPHERE_ASSERT(IsValidPriority(priority));

                        if (AMS_LIKELY(priority <= LowestPriority)) {
                            if (this->queues[priority].PushBack(core, member)) {
                                this->available_priorities[core].SetBit(priority);
                            }
                        }
                    }

                    constexpr ALWAYS_INLINE void PushFront(s32 priority, s32 core, Member *member) {
                        MESOSPHERE_ASSERT(IsValidCore(core));
                        MESOSPHERE_ASSERT(IsValidPriority(priority));

                        if (AMS_LIKELY(priority <= LowestPriority)) {
                            if (this->queues[priority].PushFront(core, member)) {
                                this->available_priorities[core].SetBit(priority);
                            }
                        }
                    }

                    constexpr ALWAYS_INLINE void Remove(s32 priority, s32 core, Member *member) {
                        MESOSPHERE_ASSERT(IsValidCore(core));
                        MESOSPHERE_ASSERT(IsValidPriority(priority));

                        if (AMS_LIKELY(priority <= LowestPriority)) {
                            if (this->queues[priority].Remove(core, member)) {
                                this->available_priorities[core].ClearBit(priority);
                            }
                        }
                    }

                    constexpr ALWAYS_INLINE Member *GetFront(s32 core) const {
                        MESOSPHERE_ASSERT(IsValidCore(core));

                        const s32 priority = this->available_priorities[core].CountLeadingZero();
                        if (AMS_LIKELY(priority <= LowestPriority)) {
                            return this->queues[priority].GetFront(core);
                        } else {
                            return nullptr;
                        }
                    }

                    constexpr ALWAYS_INLINE Member *GetFront(s32 priority, s32 core) const {
                        MESOSPHERE_ASSERT(IsValidCore(core));
                        MESOSPHERE_ASSERT(IsValidPriority(priority));

                        if (AMS_LIKELY(priority <= LowestPriority)) {
                            return this->queues[priority].GetFront(core);
                        } else {
                            return nullptr;
                        }
                    }

                    constexpr ALWAYS_INLINE Member *GetNext(s32 core, const Member *member) const {
                        MESOSPHERE_ASSERT(IsValidCore(core));

                        Member *next = member->GetPriorityQueueEntry(core).GetNext();
                        if (next == nullptr) {
                            const s32 priority = this->available_priorities[core].GetNextSet(member->GetPriority());
                            if (AMS_LIKELY(priority <= LowestPriority)) {
                                next = this->queues[priority].GetFront(core);
                            }
                        }
                        return next;
                    }

                    constexpr ALWAYS_INLINE void MoveToFront(s32 priority, s32 core, Member *member) {
                        MESOSPHERE_ASSERT(IsValidCore(core));
                        MESOSPHERE_ASSERT(IsValidPriority(priority));

                        if (AMS_LIKELY(priority <= LowestPriority)) {
                            this->queues[priority].Remove(core, member);
                            this->queues[priority].PushFront(core, member);
                        }
                    }

                    constexpr ALWAYS_INLINE Member *MoveToBack(s32 priority, s32 core, Member *member) {
                        MESOSPHERE_ASSERT(IsValidCore(core));
                        MESOSPHERE_ASSERT(IsValidPriority(priority));

                        if (AMS_LIKELY(priority <= LowestPriority)) {
                            this->queues[priority].Remove(core, member);
                            this->queues[priority].PushBack(core, member);
                            return this->queues[priority].GetFront(core);
                        } else {
                            return nullptr;
                        }
                    }
            };
        private:
            KPriorityQueueImpl scheduled_queue;
            KPriorityQueueImpl suggested_queue;
        private:
            constexpr ALWAYS_INLINE void ClearAffinityBit(u64 &affinity, s32 core) {
                affinity &= ~(u64(1ul) << core);
            }

            constexpr ALWAYS_INLINE s32 GetNextCore(u64 &affinity) {
                const s32 core = __builtin_ctzll(static_cast<unsigned long long>(affinity));
                ClearAffinityBit(affinity, core);
                return core;
            }

            constexpr ALWAYS_INLINE void PushBack(s32 priority, Member *member) {
                MESOSPHERE_ASSERT(IsValidPriority(priority));

                /* Push onto the scheduled queue for its core, if we can. */
                u64 affinity = member->GetAffinityMask().GetAffinityMask();
                if (const s32 core = member->GetActiveCore(); core >= 0) {
                    this->scheduled_queue.PushBack(priority, core, member);
                    ClearAffinityBit(affinity, core);
                }

                /* And suggest the thread for all other cores. */
                while (affinity) {
                    this->suggested_queue.PushBack(priority, GetNextCore(affinity), member);
                }
            }

            constexpr ALWAYS_INLINE void PushFront(s32 priority, Member *member) {
                MESOSPHERE_ASSERT(IsValidPriority(priority));

                /* Push onto the scheduled queue for its core, if we can. */
                u64 affinity = member->GetAffinityMask().GetAffinityMask();
                if (const s32 core = member->GetActiveCore(); core >= 0) {
                    this->scheduled_queue.PushFront(priority, core, member);
                    ClearAffinityBit(affinity, core);
                }

                /* And suggest the thread for all other cores. */
                /* Note: Nintendo pushes onto the back of the suggested queue, not the front. */
                while (affinity) {
                    this->suggested_queue.PushBack(priority, GetNextCore(affinity), member);
                }
            }

            constexpr ALWAYS_INLINE void Remove(s32 priority, Member *member) {
                MESOSPHERE_ASSERT(IsValidPriority(priority));

                /* Remove from the scheduled queue for its core. */
                u64 affinity = member->GetAffinityMask().GetAffinityMask();
                if (const s32 core = member->GetActiveCore(); core >= 0) {
                    this->scheduled_queue.Remove(priority, core, member);
                    ClearAffinityBit(affinity, core);
                }

                /* Remove from the suggested queue for all other cores. */
                while (affinity) {
                    this->suggested_queue.Remove(priority, GetNextCore(affinity), member);
                }
            }
        public:
            constexpr ALWAYS_INLINE KPriorityQueue() : scheduled_queue(), suggested_queue() { /* ... */ }

            /* Getters. */
            constexpr ALWAYS_INLINE Member *GetScheduledFront(s32 core) const {
                return this->scheduled_queue.GetFront(core);
            }

            constexpr ALWAYS_INLINE Member *GetScheduledFront(s32 core, s32 priority) const {
                return this->scheduled_queue.GetFront(priority, core);
            }

            constexpr ALWAYS_INLINE Member *GetSuggestedFront(s32 core) const {
                return this->suggested_queue.GetFront(core);
            }

            constexpr ALWAYS_INLINE Member *GetSuggestedFront(s32 core, s32 priority) const {
                return this->suggested_queue.GetFront(priority, core);
            }

            constexpr ALWAYS_INLINE Member *GetScheduledNext(s32 core, const Member *member) const {
                return this->scheduled_queue.GetNext(core, member);
            }

            constexpr ALWAYS_INLINE Member *GetSuggestedNext(s32 core, const Member *member) const {
                return this->suggested_queue.GetNext(core, member);
            }

            constexpr ALWAYS_INLINE Member *GetSamePriorityNext(s32 core, const Member *member) const {
                return member->GetPriorityQueueEntry(core).GetNext();
            }

            /* Mutators. */
            constexpr ALWAYS_INLINE void PushBack(Member *member) {
                this->PushBack(member->GetPriority(), member);
            }

            constexpr ALWAYS_INLINE void Remove(Member *member) {
                this->Remove(member->GetPriority(), member);
            }

            constexpr ALWAYS_INLINE void MoveToScheduledFront(Member *member) {
                this->scheduled_queue.MoveToFront(member->GetPriority(), member->GetActiveCore(), member);
            }

            constexpr ALWAYS_INLINE KThread *MoveToScheduledBack(Member *member) {
                return this->scheduled_queue.MoveToBack(member->GetPriority(), member->GetActiveCore(), member);
            }

            /* First class fancy operations. */
            constexpr ALWAYS_INLINE void ChangePriority(s32 prev_priority, bool is_running, Member *member) {
                MESOSPHERE_ASSERT(IsValidPriority(prev_priority));

                /* Remove the member from the queues. */
                const s32 new_priority = member->GetPriority();
                this->Remove(prev_priority, member);

                /* And enqueue. If the member is running, we want to keep it running. */
                if (is_running) {
                    this->PushFront(new_priority, member);
                } else {
                    this->PushBack(new_priority, member);
                }
            }

            constexpr ALWAYS_INLINE void ChangeAffinityMask(s32 prev_core, const AffinityMaskType &prev_affinity, Member *member) {
                /* Get the new information. */
                const s32 priority                   = member->GetPriority();
                const AffinityMaskType &new_affinity = member->GetAffinityMask();
                const s32 new_core                   = member->GetActiveCore();

                /* Remove the member from all queues it was in before. */
                for (s32 core = 0; core < static_cast<s32>(NumCores); core++) {
                    if (prev_affinity.GetAffinity(core)) {
                        if (core == prev_core) {
                            this->scheduled_queue.Remove(priority, core, member);
                        } else {
                            this->suggested_queue.Remove(priority, core, member);
                        }
                    }
                }

                /* And add the member to all queues it should be in now. */
                for (s32 core = 0; core < static_cast<s32>(NumCores); core++) {
                    if (new_affinity.GetAffinity(core)) {
                        if (core == new_core) {
                            this->scheduled_queue.PushBack(priority, core, member);
                        } else {
                            this->suggested_queue.PushBack(priority, core, member);
                        }
                    }
                }
            }

            constexpr ALWAYS_INLINE void ChangeCore(s32 prev_core, Member *member, bool to_front = false) {
                /* Get the new information. */
                const s32 new_core = member->GetActiveCore();
                const s32 priority = member->GetPriority();

                /* We don't need to do anything if the core is the same. */
                if (prev_core != new_core) {
                    /* Remove from the scheduled queue for the previous core. */
                    if (prev_core >= 0) {
                        this->scheduled_queue.Remove(priority, prev_core, member);
                    }

                    /* Remove from the suggested queue and add to the scheduled queue for the new core. */
                    if (new_core >= 0) {
                        this->suggested_queue.Remove(priority, new_core, member);
                        if (to_front) {
                            this->scheduled_queue.PushFront(priority, new_core, member);
                        } else {
                            this->scheduled_queue.PushBack(priority, new_core, member);
                        }
                    }

                    /* Add to the suggested queue for the previous core. */
                    if (prev_core >= 0) {
                        this->suggested_queue.PushBack(priority, prev_core, member);
                    }
                }
            }
    };

}
