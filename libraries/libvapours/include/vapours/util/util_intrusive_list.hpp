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

#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_parent_of_member.hpp>

namespace ams::util {

    #pragma GCC push_options
    #pragma GCC optimize ("-O3")

    /* Forward declare implementation class for Node. */
    namespace impl {

        class IntrusiveListImpl;

    }

    class IntrusiveListNode {
        NON_COPYABLE(IntrusiveListNode);
        private:
            friend class impl::IntrusiveListImpl;

            IntrusiveListNode *m_prev;
            IntrusiveListNode *m_next;
        public:
            constexpr ALWAYS_INLINE IntrusiveListNode() : m_prev(this), m_next(this) { /* ... */ }

            constexpr ALWAYS_INLINE bool IsLinked() const {
                return m_next != this;
            }
        private:
            constexpr ALWAYS_INLINE void LinkPrev(IntrusiveListNode *node) {
                /* We can't link an already linked node. */
                AMS_ASSERT(!node->IsLinked());
                this->SplicePrev(node, node);
            }

            constexpr ALWAYS_INLINE void SplicePrev(IntrusiveListNode *first, IntrusiveListNode *last) {
                /* Splice a range into the list. */
                auto last_prev = last->m_prev;
                first->m_prev = m_prev;
                last_prev->m_next = this;
                m_prev->m_next = first;
                m_prev = last_prev;
            }

            constexpr ALWAYS_INLINE void LinkNext(IntrusiveListNode *node) {
                /* We can't link an already linked node. */
                AMS_ASSERT(!node->IsLinked());
                return this->SpliceNext(node, node);
            }

            constexpr ALWAYS_INLINE void SpliceNext(IntrusiveListNode *first, IntrusiveListNode *last) {
                /* Splice a range into the list. */
                auto last_prev = last->m_prev;
                first->m_prev = this;
                last_prev->m_next = m_next;
                m_next->m_prev = last_prev;
                m_next = first;
            }

            constexpr ALWAYS_INLINE void Unlink() {
                this->Unlink(m_next);
            }

            constexpr ALWAYS_INLINE void Unlink(IntrusiveListNode *last) {
                /* Unlink a node from a next node. */
                auto last_prev = last->m_prev;
                m_prev->m_next = last;
                last->m_prev = m_prev;
                last_prev->m_next = this;
                m_prev = last_prev;
            }

            constexpr ALWAYS_INLINE IntrusiveListNode *GetPrev() {
                return m_prev;
            }

            constexpr ALWAYS_INLINE const IntrusiveListNode *GetPrev() const {
                return m_prev;
            }

            constexpr ALWAYS_INLINE IntrusiveListNode *GetNext() {
                return m_next;
            }

            constexpr ALWAYS_INLINE const IntrusiveListNode *GetNext() const {
                return m_next;
            }
    };
    /* DEPRECATED: static_assert(std::is_literal_type<IntrusiveListNode>::value); */

    namespace impl {

        class IntrusiveListImpl {
            NON_COPYABLE(IntrusiveListImpl);
            private:
                IntrusiveListNode m_root_node;
            public:
                template<bool Const>
                class Iterator;

                using value_type             = IntrusiveListNode;
                using size_type              = size_t;
                using difference_type        = ptrdiff_t;
                using pointer                = value_type *;
                using const_pointer          = const value_type *;
                using reference              = value_type &;
                using const_reference        = const value_type &;
                using iterator               = Iterator<false>;
                using const_iterator         = Iterator<true>;
                using reverse_iterator       = std::reverse_iterator<iterator>;
                using const_reverse_iterator = std::reverse_iterator<const_iterator>;

                template<bool Const>
                class Iterator {
                    public:
                        using iterator_category = std::bidirectional_iterator_tag;
                        using value_type        = typename IntrusiveListImpl::value_type;
                        using difference_type   = typename IntrusiveListImpl::difference_type;
                        using pointer           = typename std::conditional<Const, IntrusiveListImpl::const_pointer, IntrusiveListImpl::pointer>::type;
                        using reference         = typename std::conditional<Const, IntrusiveListImpl::const_reference, IntrusiveListImpl::reference>::type;
                    private:
                        pointer m_node;
                    public:
                        constexpr ALWAYS_INLINE explicit Iterator(pointer n) : m_node(n) { /* ... */ }

                        constexpr ALWAYS_INLINE bool operator==(const Iterator &rhs) const {
                            return m_node == rhs.m_node;
                        }

                        constexpr ALWAYS_INLINE bool operator!=(const Iterator &rhs) const {
                            return !(*this == rhs);
                        }

                        constexpr ALWAYS_INLINE pointer operator->() const {
                            return m_node;
                        }

                        constexpr ALWAYS_INLINE reference operator*() const {
                            return *m_node;
                        }

                        constexpr ALWAYS_INLINE Iterator &operator++() {
                            m_node = m_node->m_next;
                            return *this;
                        }

                        constexpr ALWAYS_INLINE Iterator &operator--() {
                            m_node = m_node->m_prev;
                            return *this;
                        }

                        constexpr ALWAYS_INLINE Iterator operator++(int) {
                            const Iterator it{*this};
                            ++(*this);
                            return it;
                        }

                        constexpr ALWAYS_INLINE Iterator operator--(int) {
                            const Iterator it{*this};
                            --(*this);
                            return it;
                        }

                        constexpr ALWAYS_INLINE operator Iterator<true>() const {
                            return Iterator<true>(m_node);
                        }

                        constexpr ALWAYS_INLINE Iterator<false> GetNonConstIterator() const {
                            return Iterator<false>(const_cast<IntrusiveListImpl::pointer>(m_node));
                        }
                };
            public:
                constexpr ALWAYS_INLINE IntrusiveListImpl() : m_root_node() { /* ... */ }

                /* Iterator accessors. */
                constexpr ALWAYS_INLINE iterator begin() {
                    return iterator(m_root_node.GetNext());
                }

                constexpr ALWAYS_INLINE const_iterator begin() const {
                    return const_iterator(m_root_node.GetNext());
                }

                constexpr ALWAYS_INLINE iterator end() {
                    return iterator(std::addressof(m_root_node));
                }

                constexpr ALWAYS_INLINE const_iterator end() const {
                    return const_iterator(std::addressof(m_root_node));
                }

                constexpr ALWAYS_INLINE iterator iterator_to(reference v) {
                    /* Only allow iterator_to for values in lists. */
                    AMS_ASSERT(v.IsLinked());
                    return iterator(std::addressof(v));
                }

                constexpr ALWAYS_INLINE const_iterator iterator_to(const_reference v) const {
                    /* Only allow iterator_to for values in lists. */
                    AMS_ASSERT(v.IsLinked());
                    return const_iterator(std::addressof(v));
                }

                /* Content management. */
                constexpr ALWAYS_INLINE bool empty() const {
                    return !m_root_node.IsLinked();
                }

                constexpr ALWAYS_INLINE size_type size() const {
                    return static_cast<size_type>(std::distance(this->begin(), this->end()));
                }

                constexpr ALWAYS_INLINE reference back() {
                    return *m_root_node.GetPrev();
                }

                constexpr ALWAYS_INLINE const_reference back() const {
                    return *m_root_node.GetPrev();
                }

                constexpr ALWAYS_INLINE reference front() {
                    return *m_root_node.GetNext();
                }

                constexpr ALWAYS_INLINE const_reference front() const {
                    return *m_root_node.GetNext();
                }

                constexpr ALWAYS_INLINE void push_back(reference node) {
                    m_root_node.LinkPrev(std::addressof(node));
                }

                constexpr ALWAYS_INLINE void push_front(reference node) {
                    m_root_node.LinkNext(std::addressof(node));
                }

                constexpr ALWAYS_INLINE void pop_back() {
                    m_root_node.GetPrev()->Unlink();
                }

                constexpr ALWAYS_INLINE void pop_front() {
                    m_root_node.GetNext()->Unlink();
                }

                constexpr ALWAYS_INLINE iterator insert(const_iterator pos, reference node) {
                    pos.GetNonConstIterator()->LinkPrev(std::addressof(node));
                    return iterator(std::addressof(node));
                }

                constexpr ALWAYS_INLINE void splice(const_iterator pos, IntrusiveListImpl &o) {
                    splice_impl(pos, o.begin(), o.end());
                }

                constexpr ALWAYS_INLINE void splice(const_iterator pos, IntrusiveListImpl &o, const_iterator first) {
                    AMS_UNUSED(o);
                    const_iterator last(first);
                    std::advance(last, 1);
                    splice_impl(pos, first, last);
                }

                constexpr ALWAYS_INLINE void splice(const_iterator pos, IntrusiveListImpl &o, const_iterator first, const_iterator last) {
                    AMS_UNUSED(o);
                    splice_impl(pos, first, last);
                }

                constexpr ALWAYS_INLINE iterator erase(const_iterator pos) {
                    if (pos == this->end()) {
                        return this->end();
                    }
                    iterator it(pos.GetNonConstIterator());
                    (it++)->Unlink();
                    return it;
                }

                constexpr ALWAYS_INLINE void clear() {
                    while (!this->empty()) {
                        this->pop_front();
                    }
                }
            private:
                constexpr ALWAYS_INLINE void splice_impl(const_iterator _pos, const_iterator _first, const_iterator _last) {
                    if (_first == _last) {
                        return;
                    }
                    iterator pos(_pos.GetNonConstIterator());
                    iterator first(_first.GetNonConstIterator());
                    iterator last(_last.GetNonConstIterator());
                    first->Unlink(std::addressof(*last));
                    pos->SplicePrev(std::addressof(*first), std::addressof(*first));
                }
        };

    }

    template<class T, class Traits>
    class IntrusiveList {
        NON_COPYABLE(IntrusiveList);
        private:
            impl::IntrusiveListImpl m_impl;
        public:
            template<bool Const>
            class Iterator;

            using value_type             = T;
            using size_type              = size_t;
            using difference_type        = ptrdiff_t;
            using pointer                = value_type *;
            using const_pointer          = const value_type *;
            using reference              = value_type &;
            using const_reference        = const value_type &;
            using iterator               = Iterator<false>;
            using const_iterator         = Iterator<true>;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

            template<bool Const>
            class Iterator {
                public:
                    friend class ams::util::IntrusiveList<T, Traits>;

                    using ImplIterator = typename std::conditional<Const, ams::util::impl::IntrusiveListImpl::const_iterator, ams::util::impl::IntrusiveListImpl::iterator>::type;

                    using iterator_category = std::bidirectional_iterator_tag;
                    using value_type        = typename IntrusiveList::value_type;
                    using difference_type   = typename IntrusiveList::difference_type;
                    using pointer           = typename std::conditional<Const, IntrusiveList::const_pointer, IntrusiveList::pointer>::type;
                    using reference         = typename std::conditional<Const, IntrusiveList::const_reference, IntrusiveList::reference>::type;
                private:
                    ImplIterator m_iterator;
                private:
                    constexpr explicit ALWAYS_INLINE Iterator(ImplIterator it) : m_iterator(it) { /* ... */ }

                    constexpr ALWAYS_INLINE ImplIterator GetImplIterator() const {
                        return m_iterator;
                    }
                public:
                    constexpr ALWAYS_INLINE bool operator==(const Iterator &rhs) const {
                        return m_iterator == rhs.m_iterator;
                    }

                    constexpr ALWAYS_INLINE bool operator!=(const Iterator &rhs) const {
                        return !(*this == rhs);
                    }

                    constexpr ALWAYS_INLINE pointer operator->() const {
                        return std::addressof(Traits::GetParent(*m_iterator));
                    }

                    constexpr ALWAYS_INLINE reference operator*() const {
                        return Traits::GetParent(*m_iterator);
                    }

                    constexpr ALWAYS_INLINE Iterator &operator++() {
                        ++m_iterator;
                        return *this;
                    }

                    constexpr ALWAYS_INLINE Iterator &operator--() {
                        --m_iterator;
                        return *this;
                    }

                    constexpr ALWAYS_INLINE Iterator operator++(int) {
                        const Iterator it{*this};
                        ++m_iterator;
                        return it;
                    }

                    constexpr ALWAYS_INLINE Iterator operator--(int) {
                        const Iterator it{*this};
                        --m_iterator;
                        return it;
                    }

                    constexpr ALWAYS_INLINE operator Iterator<true>() const {
                        return Iterator<true>(m_iterator);
                    }
            };
        private:
            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(reference ref) {
                return Traits::GetNode(ref);
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(const_reference ref) {
                return Traits::GetNode(ref);
            }

            static constexpr ALWAYS_INLINE reference GetParent(IntrusiveListNode &node) {
                return Traits::GetParent(node);
            }

            static constexpr ALWAYS_INLINE const_reference GetParent(IntrusiveListNode const &node) {
                return Traits::GetParent(node);
            }
        public:
            constexpr ALWAYS_INLINE IntrusiveList() : m_impl() { /* ... */ }

            /* Iterator accessors. */
            constexpr ALWAYS_INLINE iterator begin() {
                return iterator(m_impl.begin());
            }

            constexpr ALWAYS_INLINE const_iterator begin() const {
                return const_iterator(m_impl.begin());
            }

            constexpr ALWAYS_INLINE iterator end() {
                return iterator(m_impl.end());
            }

            constexpr ALWAYS_INLINE const_iterator end() const {
                return const_iterator(m_impl.end());
            }

            constexpr ALWAYS_INLINE const_iterator cbegin() const {
                return this->begin();
            }

            constexpr ALWAYS_INLINE const_iterator cend() const {
                return this->end();
            }

            constexpr ALWAYS_INLINE reverse_iterator rbegin() {
                return reverse_iterator(this->end());
            }

            constexpr ALWAYS_INLINE const_reverse_iterator rbegin() const {
                return const_reverse_iterator(this->end());
            }

            constexpr ALWAYS_INLINE reverse_iterator rend() {
                return reverse_iterator(this->begin());
            }

            constexpr ALWAYS_INLINE const_reverse_iterator rend() const {
                return const_reverse_iterator(this->begin());
            }

            constexpr ALWAYS_INLINE const_reverse_iterator crbegin() const {
                return this->rbegin();
            }

            constexpr ALWAYS_INLINE const_reverse_iterator crend() const {
                return this->rend();
            }

            constexpr ALWAYS_INLINE iterator iterator_to(reference v) {
                return iterator(m_impl.iterator_to(GetNode(v)));
            }

            constexpr ALWAYS_INLINE const_iterator iterator_to(const_reference v) const {
                return const_iterator(m_impl.iterator_to(GetNode(v)));
            }

            /* Content management. */
            constexpr ALWAYS_INLINE bool empty() const {
                return m_impl.empty();
            }

            constexpr ALWAYS_INLINE size_type size() const {
                return m_impl.size();
            }

            constexpr ALWAYS_INLINE reference back() {
                AMS_ASSERT(!m_impl.empty());
                return GetParent(m_impl.back());
            }

            constexpr ALWAYS_INLINE const_reference back() const {
                AMS_ASSERT(!m_impl.empty());
                return GetParent(m_impl.back());
            }

            constexpr ALWAYS_INLINE reference front() {
                AMS_ASSERT(!m_impl.empty());
                return GetParent(m_impl.front());
            }

            constexpr ALWAYS_INLINE const_reference front() const {
                AMS_ASSERT(!m_impl.empty());
                return GetParent(m_impl.front());
            }

            constexpr ALWAYS_INLINE void push_back(reference ref) {
                m_impl.push_back(GetNode(ref));
            }

            constexpr ALWAYS_INLINE void push_front(reference ref) {
                m_impl.push_front(GetNode(ref));
            }

            constexpr ALWAYS_INLINE void pop_back() {
                AMS_ASSERT(!m_impl.empty());
                m_impl.pop_back();
            }

            constexpr ALWAYS_INLINE void pop_front() {
                AMS_ASSERT(!m_impl.empty());
                m_impl.pop_front();
            }

            constexpr ALWAYS_INLINE iterator insert(const_iterator pos, reference ref) {
                return iterator(m_impl.insert(pos.GetImplIterator(), GetNode(ref)));
            }

            constexpr ALWAYS_INLINE void splice(const_iterator pos, IntrusiveList &o) {
                m_impl.splice(pos.GetImplIterator(), o.m_impl);
            }

            constexpr ALWAYS_INLINE void splice(const_iterator pos, IntrusiveList &o, const_iterator first) {
                m_impl.splice(pos.GetImplIterator(), o.m_impl, first.GetImplIterator());
            }

            constexpr ALWAYS_INLINE void splice(const_iterator pos, IntrusiveList &o, const_iterator first, const_iterator last) {
                m_impl.splice(pos.GetImplIterator(), o.m_impl, first.GetImplIterator(), last.GetImplIterator());
            }

            constexpr ALWAYS_INLINE iterator erase(const_iterator pos) {
                return iterator(m_impl.erase(pos.GetImplIterator()));
            }

            constexpr ALWAYS_INLINE void clear() {
                m_impl.clear();
            }
    };

    template<auto T, class Derived = util::impl::GetParentType<T>>
    class IntrusiveListMemberTraits;

    template<class Parent, IntrusiveListNode Parent::*Member, class Derived>
    class IntrusiveListMemberTraits<Member, Derived> {
        public:
            using ListType = IntrusiveList<Derived, IntrusiveListMemberTraits>;
        private:
            friend class IntrusiveList<Derived, IntrusiveListMemberTraits>;

            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(Derived &parent) {
                return parent.*Member;
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(Derived const &parent) {
                return parent.*Member;
            }

            static ALWAYS_INLINE Derived &GetParent(IntrusiveListNode &node) {
                return util::GetParentReference<Member, Derived>(std::addressof(node));
            }

            static ALWAYS_INLINE Derived const &GetParent(IntrusiveListNode const &node) {
                return util::GetParentReference<Member, Derived>(std::addressof(node));
            }
    };

    template<auto T, class Derived = util::impl::GetParentType<T>>
    class IntrusiveListMemberTraitsByNonConstexprOffsetOf;

    template<class Parent, IntrusiveListNode Parent::*Member, class Derived>
    class IntrusiveListMemberTraitsByNonConstexprOffsetOf<Member, Derived> {
        public:
            using ListType = IntrusiveList<Derived, IntrusiveListMemberTraitsByNonConstexprOffsetOf>;
        private:
            friend class IntrusiveList<Derived, IntrusiveListMemberTraitsByNonConstexprOffsetOf>;

            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(Derived &parent) {
                return parent.*Member;
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(Derived const &parent) {
                return parent.*Member;
            }

            static ALWAYS_INLINE Derived &GetParent(IntrusiveListNode &node) {
                return *reinterpret_cast<Derived *>(reinterpret_cast<char *>(std::addressof(node)) - GetOffset());
            }

            static ALWAYS_INLINE Derived const &GetParent(IntrusiveListNode const &node) {
                return *reinterpret_cast<const Derived *>(reinterpret_cast<const char *>(std::addressof(node)) - GetOffset());
            }

            static ALWAYS_INLINE uintptr_t GetOffset() {
                return reinterpret_cast<uintptr_t>(std::addressof(reinterpret_cast<Derived *>(0)->*Member));
            }
    };

    template<class Derived>
    class IntrusiveListBaseNode : public IntrusiveListNode{};

    template<class Derived>
    class IntrusiveListBaseTraits {
        public:
            using ListType = IntrusiveList<Derived, IntrusiveListBaseTraits>;
        private:
            friend class IntrusiveList<Derived, IntrusiveListBaseTraits>;

            static constexpr ALWAYS_INLINE IntrusiveListNode &GetNode(Derived &parent) {
                return static_cast<IntrusiveListNode &>(static_cast<IntrusiveListBaseNode<Derived> &>(parent));
            }

            static constexpr ALWAYS_INLINE IntrusiveListNode const &GetNode(Derived const &parent) {
                return static_cast<const IntrusiveListNode &>(static_cast<const IntrusiveListBaseNode<Derived> &>(parent));
            }

            static constexpr ALWAYS_INLINE Derived &GetParent(IntrusiveListNode &node) {
                return static_cast<Derived &>(static_cast<IntrusiveListBaseNode<Derived> &>(node));
            }

            static constexpr ALWAYS_INLINE Derived const &GetParent(IntrusiveListNode const &node) {
                return static_cast<const Derived &>(static_cast<const IntrusiveListBaseNode<Derived> &>(node));
            }
    };

    #pragma GCC pop_options

}