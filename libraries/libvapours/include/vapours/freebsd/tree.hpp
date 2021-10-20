/*	$NetBSD: tree.h,v 1.8 2004/03/28 19:38:30 provos Exp $	*/
/*	$OpenBSD: tree.h,v 1.7 2002/10/17 21:51:54 art Exp $	*/
/* $FreeBSD$ */
/*-
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_type_traits.hpp>

#pragma GCC push_options
#pragma GCC optimize ("-O3")

/*
 * This file defines data structures for red-black trees.
 *
 * A red-black tree is a binary search tree with the node color as an
 * extra attribute.  It fulfills a set of conditions:
 *	- every search path from the root to a leaf consists of the
 *	  same number of black nodes,
 *	- each red node (except for the root) has a black parent,
 *	- each leaf node is black.
 *
 * Every operation on a red-black tree is bounded as O(lg n).
 * The maximum height of a red-black tree is 2lg (n+1).
 */

namespace ams::freebsd {

    enum class RBColor {
        RB_BLACK = 0,
        RB_RED   = 1,
    };

    #pragma pack(push, 4)
    template<typename T>
    class RBEntry {
        private:
            T *m_rbe_left ;
            T *m_rbe_right;
            T *m_rbe_parent;
            RBColor m_rbe_color;
        public:
            constexpr ALWAYS_INLINE explicit RBEntry(util::ConstantInitializeTag) : m_rbe_left(nullptr), m_rbe_right(nullptr), m_rbe_parent(nullptr), m_rbe_color(RBColor::RB_BLACK) { /* ... */ }
            explicit ALWAYS_INLINE RBEntry() { /* ... */ }

            [[nodiscard]] constexpr ALWAYS_INLINE       T *Left()       { return m_rbe_left; }
            [[nodiscard]] constexpr ALWAYS_INLINE const T *Left() const { return m_rbe_left; }

            constexpr ALWAYS_INLINE void SetLeft(T *e) { m_rbe_left = e; }

            [[nodiscard]] constexpr ALWAYS_INLINE       T *Right()       { return m_rbe_right; }
            [[nodiscard]] constexpr ALWAYS_INLINE const T *Right() const { return m_rbe_right; }

            constexpr ALWAYS_INLINE void SetRight(T *e) { m_rbe_right = e; }

            [[nodiscard]] constexpr ALWAYS_INLINE       T *Parent()       { return m_rbe_parent; }
            [[nodiscard]] constexpr ALWAYS_INLINE const T *Parent() const { return m_rbe_parent; }

            constexpr ALWAYS_INLINE void SetParent(T *e) { m_rbe_parent = e; }

            [[nodiscard]] constexpr ALWAYS_INLINE bool IsBlack()   const { return m_rbe_color == RBColor::RB_BLACK; }
            [[nodiscard]] constexpr ALWAYS_INLINE bool IsRed()     const { return m_rbe_color == RBColor::RB_RED; }
            [[nodiscard]] constexpr ALWAYS_INLINE RBColor Color() const { return m_rbe_color; }

            constexpr ALWAYS_INLINE void SetColor(RBColor c) { m_rbe_color = c; }
    };
    #pragma pack(pop)

    template<typename T> struct CheckRBEntry             { static constexpr bool value = false; };
    template<typename T> struct CheckRBEntry<RBEntry<T>> { static constexpr bool value = true;  };

    template<typename T>
    concept IsRBEntry = CheckRBEntry<T>::value;

    template<typename T>
    concept HasRBEntry = requires (T &t, const T &ct) {
        {  t.GetRBEntry() } -> std::same_as<      RBEntry<T> &>;
        { ct.GetRBEntry() } -> std::same_as<const RBEntry<T> &>;
    };

    template<typename T> requires HasRBEntry<T>
    class RBHead {
        private:
            T *m_rbh_root = nullptr;
        public:
            [[nodiscard]] constexpr ALWAYS_INLINE       T *Root()       { return m_rbh_root; }
            [[nodiscard]] constexpr ALWAYS_INLINE const T *Root() const { return m_rbh_root; }
            constexpr ALWAYS_INLINE void SetRoot(T *root) { m_rbh_root = root; }

            [[nodiscard]] constexpr ALWAYS_INLINE bool IsEmpty() const { return this->Root() == nullptr; }
    };

    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE       RBEntry<T> &RB_ENTRY(      T *t) { return t->GetRBEntry(); }
    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE const RBEntry<T> &RB_ENTRY(const T *t) { return t->GetRBEntry(); }

    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE       T *RB_LEFT(      T *t) { return RB_ENTRY(t).Left(); }
    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE const T *RB_LEFT(const T *t) { return RB_ENTRY(t).Left(); }

    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE       T *RB_RIGHT(      T *t) { return RB_ENTRY(t).Right(); }
    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE const T *RB_RIGHT(const T *t) { return RB_ENTRY(t).Right(); }

    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE       T *RB_PARENT(      T *t) { return RB_ENTRY(t).Parent(); }
    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE const T *RB_PARENT(const T *t) { return RB_ENTRY(t).Parent(); }

    template<typename T> requires HasRBEntry<T> constexpr ALWAYS_INLINE void RB_SET_LEFT(T *t, T *e)   { RB_ENTRY(t).SetLeft(e); }
    template<typename T> requires HasRBEntry<T> constexpr ALWAYS_INLINE void RB_SET_RIGHT(T *t, T *e)  { RB_ENTRY(t).SetRight(e); }
    template<typename T> requires HasRBEntry<T> constexpr ALWAYS_INLINE void RB_SET_PARENT(T *t, T *e) { RB_ENTRY(t).SetParent(e); }

    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE bool RB_IS_BLACK(const T *t)  { return RB_ENTRY(t).IsBlack(); }
    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE bool RB_IS_RED(const T *t)    { return RB_ENTRY(t).IsRed(); }

    template<typename T> requires HasRBEntry<T> [[nodiscard]] constexpr ALWAYS_INLINE RBColor RB_COLOR(const T *t) { return RB_ENTRY(t).Color(); }

    template<typename T> requires HasRBEntry<T> constexpr ALWAYS_INLINE void RB_SET_COLOR(T *t, RBColor c) { RB_ENTRY(t).SetColor(c); }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE void RB_SET(T *elm, T *parent) {
        auto &rb_entry = RB_ENTRY(elm);
        rb_entry.SetParent(parent);
        rb_entry.SetLeft(nullptr);
        rb_entry.SetRight(nullptr);
        rb_entry.SetColor(RBColor::RB_RED);
    }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE void RB_SET_BLACKRED(T *black, T *red) {
        RB_SET_COLOR(black, RBColor::RB_BLACK);
        RB_SET_COLOR(red, RBColor::RB_RED);
    }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE void RB_ROTATE_LEFT(RBHead<T> &head, T *elm, T *&tmp) {
        tmp = RB_RIGHT(elm);
        if (RB_SET_RIGHT(elm, RB_LEFT(tmp)); RB_RIGHT(elm) != nullptr) {
            RB_SET_PARENT(RB_LEFT(tmp), elm);
        }

        if (RB_SET_PARENT(tmp, RB_PARENT(elm)); RB_PARENT(tmp) != nullptr) {
            if (elm == RB_LEFT(RB_PARENT(elm))) {
                RB_SET_LEFT(RB_PARENT(elm), tmp);
            } else {
                RB_SET_RIGHT(RB_PARENT(elm), tmp);
            }
        } else {
            head.SetRoot(tmp);
        }

        RB_SET_LEFT(tmp, elm);
        RB_SET_PARENT(elm, tmp);
    }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE void RB_ROTATE_RIGHT(RBHead<T> &head, T *elm, T *&tmp) {
        tmp = RB_LEFT(elm);
        if (RB_SET_LEFT(elm, RB_RIGHT(tmp)); RB_LEFT(elm) != nullptr) {
            RB_SET_PARENT(RB_RIGHT(tmp), elm);
        }

        if (RB_SET_PARENT(tmp, RB_PARENT(elm)); RB_PARENT(tmp) != nullptr) {
            if (elm == RB_LEFT(RB_PARENT(elm))) {
                RB_SET_LEFT(RB_PARENT(elm), tmp);
            } else {
                RB_SET_RIGHT(RB_PARENT(elm), tmp);
            }
        } else {
            head.SetRoot(tmp);
        }

        RB_SET_RIGHT(tmp, elm);
        RB_SET_PARENT(elm, tmp);
    }

    template <typename T> requires HasRBEntry<T>
    constexpr void RB_REMOVE_COLOR(RBHead<T> &head, T *parent, T *elm) {
        T *tmp;
        while ((elm == nullptr || RB_IS_BLACK(elm)) && elm != head.Root()) {
            if (RB_LEFT(parent) == elm) {
                tmp = RB_RIGHT(parent);
                if (RB_IS_RED(tmp)) {
                    RB_SET_BLACKRED(tmp, parent);
                    RB_ROTATE_LEFT(head, parent, tmp);
                    tmp = RB_RIGHT(parent);
                }

                if ((RB_LEFT(tmp) == nullptr || RB_IS_BLACK(RB_LEFT(tmp))) &&
                    (RB_RIGHT(tmp) == nullptr || RB_IS_BLACK(RB_RIGHT(tmp)))) {
                    RB_SET_COLOR(tmp, RBColor::RB_RED);
                    elm = parent;
                    parent = RB_PARENT(elm);
                } else {
                    if (RB_RIGHT(tmp) == nullptr || RB_IS_BLACK(RB_RIGHT(tmp))) {
                        T *oleft;
                        if ((oleft = RB_LEFT(tmp)) != nullptr) {
                            RB_SET_COLOR(oleft, RBColor::RB_BLACK);
                        }

                        RB_SET_COLOR(tmp, RBColor::RB_RED);
                        RB_ROTATE_RIGHT(head, tmp, oleft);
                        tmp = RB_RIGHT(parent);
                    }

                    RB_SET_COLOR(tmp, RB_COLOR(parent));
                    RB_SET_COLOR(parent, RBColor::RB_BLACK);
                    if (RB_RIGHT(tmp)) {
                        RB_SET_COLOR(RB_RIGHT(tmp), RBColor::RB_BLACK);
                    }

                    RB_ROTATE_LEFT(head, parent, tmp);
                    elm = head.Root();
                    break;
                }
            } else {
                tmp = RB_LEFT(parent);
                if (RB_IS_RED(tmp)) {
                    RB_SET_BLACKRED(tmp, parent);
                    RB_ROTATE_RIGHT(head, parent, tmp);
                    tmp = RB_LEFT(parent);
                }

                if ((RB_LEFT(tmp) == nullptr || RB_IS_BLACK(RB_LEFT(tmp))) &&
                    (RB_RIGHT(tmp) == nullptr || RB_IS_BLACK(RB_RIGHT(tmp)))) {
                    RB_SET_COLOR(tmp, RBColor::RB_RED);
                    elm = parent;
                    parent = RB_PARENT(elm);
                } else {
                    if (RB_LEFT(tmp) == nullptr || RB_IS_BLACK(RB_LEFT(tmp))) {
                        T *oright;
                        if ((oright = RB_RIGHT(tmp)) != nullptr) {
                            RB_SET_COLOR(oright, RBColor::RB_BLACK);
                        }

                        RB_SET_COLOR(tmp, RBColor::RB_RED);
                        RB_ROTATE_LEFT(head, tmp, oright);
                        tmp = RB_LEFT(parent);
                    }

                    RB_SET_COLOR(tmp, RB_COLOR(parent));
                    RB_SET_COLOR(parent, RBColor::RB_BLACK);

                    if (RB_LEFT(tmp)) {
                        RB_SET_COLOR(RB_LEFT(tmp), RBColor::RB_BLACK);
                    }

                    RB_ROTATE_RIGHT(head, parent, tmp);
                    elm = head.Root();
                    break;
                }
            }
        }

        if (elm) {
            RB_SET_COLOR(elm, RBColor::RB_BLACK);
        }
    }

    template <typename T> requires HasRBEntry<T>
    constexpr T *RB_REMOVE(RBHead<T> &head, T *elm) {
        T *child      = nullptr;
        T *parent     = nullptr;
        T *old        = elm;
        RBColor color = RBColor::RB_BLACK;

        if (RB_LEFT(elm) == nullptr) {
            child = RB_RIGHT(elm);
        } else if (RB_RIGHT(elm) == nullptr) {
            child = RB_LEFT(elm);
        } else {
            T *left;
            elm = RB_RIGHT(elm);
            while ((left = RB_LEFT(elm)) != nullptr) {
                elm = left;
            }

            child = RB_RIGHT(elm);
            parent = RB_PARENT(elm);
            color = RB_COLOR(elm);

            if (child) {
                RB_SET_PARENT(child, parent);
            }

            if (parent) {
                if (RB_LEFT(parent) == elm) {
                    RB_SET_LEFT(parent, child);
                } else {
                    RB_SET_RIGHT(parent, child);
                }
            } else {
                head.SetRoot(child);
            }

            if (RB_PARENT(elm) == old) {
                parent = elm;
            }

            elm->SetRBEntry(old->GetRBEntry());

            if (RB_PARENT(old)) {
                if (RB_LEFT(RB_PARENT(old)) == old) {
                    RB_SET_LEFT(RB_PARENT(old), elm);
                } else {
                    RB_SET_RIGHT(RB_PARENT(old), elm);
                }
            } else {
                head.SetRoot(elm);
            }

            RB_SET_PARENT(RB_LEFT(old), elm);

            if (RB_RIGHT(old)) {
                RB_SET_PARENT(RB_RIGHT(old), elm);
            }

            if (parent) {
                left = parent;
            }

            if (color == RBColor::RB_BLACK) {
                RB_REMOVE_COLOR(head, parent, child);
            }

            return old;
        }

        parent = RB_PARENT(elm);
        color = RB_COLOR(elm);

        if (child) {
            RB_SET_PARENT(child, parent);
        }
        if (parent) {
            if (RB_LEFT(parent) == elm) {
                RB_SET_LEFT(parent, child);
            } else {
                RB_SET_RIGHT(parent, child);
            }
        } else {
            head.SetRoot(child);
        }

        if (color == RBColor::RB_BLACK) {
            RB_REMOVE_COLOR(head, parent, child);
        }

        return old;
    }

    template<typename T> requires HasRBEntry<T>
    constexpr void RB_INSERT_COLOR(RBHead<T> &head, T *elm) {
        T *parent = nullptr, *tmp = nullptr;
        while ((parent = RB_PARENT(elm)) != nullptr && RB_IS_RED(parent)) {
            T *gparent = RB_PARENT(parent);
            if (parent == RB_LEFT(gparent)) {
                tmp = RB_RIGHT(gparent);
                if (tmp && RB_IS_RED(tmp)) {
                    RB_SET_COLOR(tmp, RBColor::RB_BLACK);
                    RB_SET_BLACKRED(parent, gparent);
                    elm = gparent;
                    continue;
                }

                if (RB_RIGHT(parent) == elm) {
                    RB_ROTATE_LEFT(head, parent, tmp);
                    tmp = parent;
                    parent = elm;
                    elm = tmp;
                }

                RB_SET_BLACKRED(parent, gparent);
                RB_ROTATE_RIGHT(head, gparent, tmp);
            } else {
                tmp = RB_LEFT(gparent);
                if (tmp && RB_IS_RED(tmp)) {
                    RB_SET_COLOR(tmp, RBColor::RB_BLACK);
                    RB_SET_BLACKRED(parent, gparent);
                    elm = gparent;
                    continue;
                }

                if (RB_LEFT(parent) == elm) {
                    RB_ROTATE_RIGHT(head, parent, tmp);
                    tmp = parent;
                    parent = elm;
                    elm = tmp;
                }

                RB_SET_BLACKRED(parent, gparent);
                RB_ROTATE_LEFT(head, gparent, tmp);
            }
        }

        RB_SET_COLOR(head.Root(), RBColor::RB_BLACK);
    }

    template <typename T, typename Compare> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_INSERT(RBHead<T> &head, T *elm, Compare cmp) {
        T *parent = nullptr;
        T *tmp    = head.Root();
        int comp  = 0;

        while (tmp) {
            parent = tmp;
            comp   = cmp(elm, parent);
            if (comp < 0) {
                tmp = RB_LEFT(tmp);
            } else if (comp > 0) {
                tmp = RB_RIGHT(tmp);
            } else {
                return tmp;
            }
        }

        RB_SET(elm, parent);

        if (parent != nullptr) {
            if (comp < 0) {
                RB_SET_LEFT(parent, elm);
            } else {
                RB_SET_RIGHT(parent, elm);
            }
        } else {
            head.SetRoot(elm);
        }

        RB_INSERT_COLOR(head, elm);
        return nullptr;
    }

    template<typename T, typename Compare> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_FIND(RBHead<T> &head, T *elm, Compare cmp) {
        T *tmp = head.Root();

        while (tmp) {
            const int comp = cmp(elm, tmp);
            if (comp < 0) {
                tmp = RB_LEFT(tmp);
            } else if (comp > 0) {
                tmp = RB_RIGHT(tmp);
            } else {
                return tmp;
            }
        }

        return nullptr;
    }

    template<typename T, typename Compare> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_NFIND(RBHead<T> &head, T *elm, Compare cmp) {
        T *tmp = head.Root();
        T* res = nullptr;

        while (tmp) {
            const int comp = cmp(elm, tmp);
            if (comp < 0) {
                res = tmp;
                tmp = RB_LEFT(tmp);
            } else if (comp > 0) {
                tmp = RB_RIGHT(tmp);
            } else {
                return tmp;
            }
        }

        return res;
    }

    template<typename T, typename U, typename Compare> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_FIND_KEY(RBHead<T> &head, const U &key, Compare cmp) {
        T *tmp = head.Root();

        while (tmp) {
            const int comp = cmp(key, tmp);
            if (comp < 0) {
                tmp = RB_LEFT(tmp);
            } else if (comp > 0) {
                tmp = RB_RIGHT(tmp);
            } else {
                return tmp;
            }
        }

        return nullptr;
    }

    template<typename T, typename U, typename Compare> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_NFIND_KEY(RBHead<T> &head, const U &key, Compare cmp) {
        T *tmp = head.Root();
        T* res = nullptr;

        while (tmp) {
            const int comp = cmp(key, tmp);
            if (comp < 0) {
                res = tmp;
                tmp = RB_LEFT(tmp);
            } else if (comp > 0) {
                tmp = RB_RIGHT(tmp);
            } else {
                return tmp;
            }
        }

        return res;
    }

    template<typename T, typename Compare> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_FIND_EXISTING(RBHead<T> &head, T *elm, Compare cmp) {
        T *tmp = head.Root();

        while (true) {
            const int comp = cmp(elm, tmp);
            if (comp < 0) {
                tmp = RB_LEFT(tmp);
            } else if (comp > 0) {
                tmp = RB_RIGHT(tmp);
            } else {
                return tmp;
            }
        }
    }

    template<typename T, typename U, typename Compare> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_FIND_EXISTING_KEY(RBHead<T> &head, const U &key, Compare cmp) {
        T *tmp = head.Root();

        while (true) {
            const int comp = cmp(key, tmp);
            if (comp < 0) {
                tmp = RB_LEFT(tmp);
            } else if (comp > 0) {
                tmp = RB_RIGHT(tmp);
            } else {
                return tmp;
            }
        }
    }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_NEXT(T *elm) {
        if (RB_RIGHT(elm)) {
            elm = RB_RIGHT(elm);
            while (RB_LEFT(elm)) {
                elm = RB_LEFT(elm);
            }
        } else {
            if (RB_PARENT(elm) && (elm == RB_LEFT(RB_PARENT(elm)))) {
                elm = RB_PARENT(elm);
            } else {
                while (RB_PARENT(elm) && (elm == RB_RIGHT(RB_PARENT(elm)))) {
                    elm = RB_PARENT(elm);
                }
                elm = RB_PARENT(elm);
            }
        }
        return elm;
    }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_PREV(T *elm) {
        if (RB_LEFT(elm)) {
            elm = RB_LEFT(elm);
            while (RB_RIGHT(elm)) {
                elm = RB_RIGHT(elm);
            }
        } else {
            if (RB_PARENT(elm) && (elm == RB_RIGHT(RB_PARENT(elm)))) {
                elm = RB_PARENT(elm);
            } else {
                while (RB_PARENT(elm) && (elm == RB_LEFT(RB_PARENT(elm)))) {
                    elm = RB_PARENT(elm);
                }
                elm = RB_PARENT(elm);
            }
        }
        return elm;
    }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_MIN(RBHead<T> &head) {
        T *tmp    = head.Root();
        T *parent = nullptr;

        while (tmp) {
            parent = tmp;
            tmp    = RB_LEFT(tmp);
        }

        return parent;
    }

    template<typename T> requires HasRBEntry<T>
    constexpr ALWAYS_INLINE T *RB_MAX(RBHead<T> &head) {
        T *tmp    = head.Root();
        T *parent = nullptr;

        while (tmp) {
            parent = tmp;
            tmp    = RB_RIGHT(tmp);
        }

        return parent;
    }


}

#pragma GCC pop_options
