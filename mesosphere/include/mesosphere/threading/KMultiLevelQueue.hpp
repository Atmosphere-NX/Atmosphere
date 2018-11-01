#pragma once

#include <iterator>
#include <boost/intrusive/list.hpp>
#include <mesosphere/core/util.hpp>

namespace mesosphere
{

template<uint depth_, typename IntrusiveListType_, typename PrioGetterType_>
class KMultiLevelQueue {
    static_assert(depth_ <= 64, "Bitfield must be constrained in a u64");
public:
    static constexpr uint depth = depth_;

    using IntrusiveListType = IntrusiveListType_;
    using PrioGetterType = PrioGetterType_;

    using value_traits = typename IntrusiveListType::value_traits;

    using pointer = typename IntrusiveListType::pointer;
    using const_pointer = typename IntrusiveListType::const_pointer;
    using value_type = typename IntrusiveListType::value_type;
    using reference = typename IntrusiveListType::reference;
    using const_reference = typename IntrusiveListType::const_reference;
    using difference_type = typename IntrusiveListType::difference_type;
    using size_type = typename IntrusiveListType::size_type;

    template<bool isConst>
    class iterator_impl {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = typename KMultiLevelQueue::value_type;
            using difference_type = typename KMultiLevelQueue::difference_type;
            using pointer = typename std::conditional<
                isConst,
                typename KMultiLevelQueue::const_pointer,
                typename KMultiLevelQueue::pointer>::type;
            using reference = typename std::conditional<
                isConst,
                typename KMultiLevelQueue::const_reference,
                typename KMultiLevelQueue::reference>::type;

            bool operator==(const iterator_impl &other) const {
                return (isEnd() && other.isEnd()) || (it == other.it);
            }

            bool operator!=(const iterator_impl &other) const {
                return !(*this == other);
            }

            reference operator*() {
                return *it;
            }

            pointer operator->() {
                return it.operator->();
            }

            iterator_impl &operator++() {
                if (isEnd()) {
                    return *this;
                } else {
                    ++it;
                }
                if (it == getEndItForPrio()) {
                    u64 prios = mlq.usedPriorities;
                    prios &= ~((1ull << (currentPrio + 1)) - 1);
                    if (prios == 0) {
                        currentPrio = KMultiLevelQueue::depth;
                    } else {
                        currentPrio = __builtin_ffsll(prios) - 1;
                        it = getBeginItForPrio();
                    }
                }
                return *this;
            }

            iterator_impl &operator--() {
                if (isEnd()) {
                    if (mlq.usedPriorities != 0) {
                        currentPrio = 63 - __builtin_clzll(mlq.usedPriorities);
                        it = getEndItForPrio();
                        --it;
                    }
                } else if (it == getBeginItForPrio()) {
                    u64 prios = mlq.usedPriorities;
                    prios &= (1ull << currentPrio) - 1;
                    if (prios != 0) {
                        currentPrio = __builtin_ffsll(prios) - 1;
                        it = getEndItForPrio();
                        --it;
                    }
                } else {
                    --it;
                }
                return *this;
            }

            iterator_impl &operator++(int) {
                const iterator_impl v{*this};
                ++(*this);
                return v;
            }

            iterator_impl &operator--(int) {
                const iterator_impl v{*this};
                --(*this);
                return v;
            }

            // allow implicit const->non-const
            iterator_impl(const iterator_impl<false> &other)
                : mlq(other.mlq), it(other.it), currentPrio(other.currentPrio) {}

            friend class iterator_impl<true>;
            iterator_impl() = default;
        private:
            friend class KMultiLevelQueue;
            using container_ref = typename std::conditional<
                isConst,
                const KMultiLevelQueue &,
                KMultiLevelQueue &>::type;
            using list_iterator = typename std::conditional<
                isConst,
                typename IntrusiveListType::const_iterator,
                typename IntrusiveListType::iterator>::type;
            container_ref mlq;
            list_iterator it;
            uint currentPrio;

            explicit iterator_impl(container_ref mlq, list_iterator const &it, uint currentPrio)
                : mlq(mlq), it(it), currentPrio(currentPrio) {}
            explicit iterator_impl(container_ref mlq, uint currentPrio)
                : mlq(mlq), it(), currentPrio(currentPrio) {}
            constexpr bool isEnd() const {
                return currentPrio == KMultiLevelQueue::depth;
            }

            list_iterator getBeginItForPrio() const {
                if constexpr (isConst) {
                    return mlq.levels[currentPrio].cbegin();
                } else {
                    return mlq.levels[currentPrio].begin();
                }
            }

            list_iterator getEndItForPrio() const {
                if constexpr (isConst) {
                    return mlq.levels[currentPrio].cend();
                } else {
                    return mlq.levels[currentPrio].end();
                }
            }
    };

    using iterator = iterator_impl<false>;
    using const_iterator = iterator_impl<true>;

    void add(reference r) {
        uint prio = prioGetter(r);
        levels[prio].push_back(r);
        usedPriorities |= 1ul << prio;
    }

    void remove(const_reference r) {
        uint prio = prioGetter(r);
        levels[prio].erase(levels[prio].iterator_to(r));
        if (levels[prio].empty()) {
            usedPriorities &= ~(1ul << prio);
        }
    }

    void remove(const_iterator it) {
        remove(*it);
    }
    void erase(const_iterator it) {
        remove(it);
    }

    void adjust(const_reference r, uint oldPrio, bool isCurrentThread = false) {
        uint prio = prioGetter(r);

        // The thread is the current thread if and only if it is first on the running queue of highest priority, so it needs to be first on the dst queue as well.
        auto newnext = isCurrentThread ? levels[prio].cbegin() : levels[prio].cend();
        levels[prio].splice(newnext, levels[oldPrio], levels[oldPrio].iterator_to(r));

        usedPriorities |= 1ul << prio;
    }
    void adjust(const_iterator it, uint oldPrio, bool isCurrentThread = false) {
        adjust(*it, oldPrio, isCurrentThread);
    }

    void transferToFront(const_reference r, KMultiLevelQueue &other) {
        uint prio = prioGetter(r);
        other.levels[prio].splice(other.levels[prio].begin(), levels[prio], levels[prio].iterator_to(r));
        other.usedPriorities |= 1ul << prio;
        if (levels[prio].empty()) {
            usedPriorities &= ~(1ul << prio);
        }
    }

    void transferToFront(const_iterator it, KMultiLevelQueue &other) {
        transferToFront(*it, other);
    }

    void transferToBack(const_reference r, KMultiLevelQueue &other) {
        uint prio = prioGetter(r);
        other.levels[prio].splice(other.levels[prio].end(), levels[prio], levels[prio].iterator_to(r));
        other.usedPriorities |= 1ul << prio;
        if (levels[prio].empty()) {
            usedPriorities &= ~(1ul << prio);
        }
    }

    void transferToBack(const_iterator it, KMultiLevelQueue &other) {
        transferToBack(*it, other);
    }

    void yield(uint prio, size_type n = 1) {
        levels[prio].shift_forward(n);
    }
    void yield(const_reference r) {
        uint prio = prioGetter(r);
        if (&r == &levels[prio].front()) {
            yield(prio, 1);
        }
    }

    uint highestPrioritySet(uint maxPrio = 0) {
        u64 priorities = maxPrio == 0 ? usedPriorities : (usedPriorities & ~((1 << maxPrio) - 1));
        return priorities == 0 ? depth : (uint)(__builtin_ffsll((long long)priorities) - 1);
    }

    uint lowestPrioritySet(uint minPrio = depth - 1) {
        u64 priorities = minPrio >= depth - 1 ? usedPriorities : (usedPriorities & ((1 << (minPrio + 1)) - 1));
        return priorities == 0 ? depth : 63 - __builtin_clzll(priorities);
    }

    size_type size(uint prio) const {
        return levels[prio].size();
    }
    bool empty(uint prio) const {
        return (usedPriorities & (1 << prio)) == 0;
    }

    size_type size() const {
        u64 prios = usedPriorities;
        size_type sz = 0;
        while (prios != 0) {
            int ffs = __builtin_ffsll(prios);
            sz += size((uint)ffs - 1);
            prios &= ~(1ull << (ffs - 1));
        }

        return sz;
    }
    bool empty() const {
        return usedPriorities == 0;
    }

    reference front(uint maxPrio = 0) {
        // Undefined behavior if empty
        uint priority = highestPrioritySet(maxPrio);
        return levels[priority == depth ? 0 : priority].front();
    }
    const_reference front(uint maxPrio = 0) const {
        // Undefined behavior if empty
        uint priority = highestPrioritySet(maxPrio);
        return levels[priority == depth ? 0 : priority].front();
    }

    reference back(uint minPrio = depth - 1) {
        // Inclusive
        // Undefined behavior if empty
        uint priority = highestPrioritySet(minPrio); // intended
        return levels[priority == depth ? 63 : priority].back();
    }
    const_reference back(uint minPrio = KMultiLevelQueue::depth - 1) const {
        // Inclusive
        // Undefined behavior if empty
        uint priority = highestPrioritySet(minPrio); // intended
        return levels[priority == depth ? 63 : priority].back();
    }

    const_iterator cbegin(uint maxPrio = 0) const {
        uint priority = highestPrioritySet(maxPrio);
        return priority == depth ? cend() : const_iterator{*this, levels[priority].cbegin(), priority};
    }
    iterator begin(uint maxPrio = 0) const {
        return cbegin(maxPrio);
    }
    iterator begin(uint maxPrio = 0) {
        uint priority = highestPrioritySet(maxPrio);
        return priority == depth ? end() : iterator{*this, levels[priority].begin(), priority};
    }

    const_iterator cend(uint minPrio = depth - 1) const {
        return minPrio == depth - 1 ? const_iterator{*this, depth} : cbegin(minPrio + 1);
    }
    const_iterator end(uint minPrio = depth - 1) const {
        return cend(minPrio);
    }
    iterator end(uint minPrio = depth - 1) {
        return minPrio == depth - 1 ? iterator{*this, depth} : begin(minPrio + 1);
    }

    void swap(KMultiLevelQueue &other)
    {
        std::swap(*this, other);
    }

    KMultiLevelQueue(PrioGetterType prioGetter) : prioGetter(prioGetter), usedPriorities(0), levels() {};
    explicit KMultiLevelQueue(const value_traits &traits, PrioGetterType prioGetter = PrioGetterType{})
        : prioGetter(prioGetter), usedPriorities(0), levels(detail::MakeArrayOf<IntrusiveListType, depth>(traits)) {}

private:
    PrioGetterType prioGetter;
    u64 usedPriorities;
    std::array<IntrusiveListType, depth> levels;
};

}
