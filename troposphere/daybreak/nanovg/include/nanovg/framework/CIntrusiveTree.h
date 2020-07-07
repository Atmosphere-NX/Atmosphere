/*
** Sample Framework for deko3d Applications
**   CIntrusiveTree.h: Intrusive red-black tree helper class
*/
#pragma once
#include "common.h"

#include <functional>

struct CIntrusiveTreeNode
{
    enum Color
    {
        Red,
        Black,
    };

    enum Leaf
    {
        Left,
        Right,
    };

private:
    uintptr_t m_parent_color;
    CIntrusiveTreeNode* m_children[2];

public:
    constexpr CIntrusiveTreeNode() : m_parent_color{}, m_children{} { }

    constexpr CIntrusiveTreeNode* getParent() const
    {
        return reinterpret_cast<CIntrusiveTreeNode*>(m_parent_color &~ 1);
    }

    void setParent(CIntrusiveTreeNode* parent)
    {
        m_parent_color = (m_parent_color & 1) | reinterpret_cast<uintptr_t>(parent);
    }

    constexpr Color getColor() const
    {
        return static_cast<Color>(m_parent_color & 1);
    }

    void setColor(Color color)
    {
        m_parent_color = (m_parent_color &~ 1) | static_cast<uintptr_t>(color);
    }

    constexpr CIntrusiveTreeNode*& child(Leaf leaf)
    {
        return m_children[leaf];
    }

    constexpr CIntrusiveTreeNode* const& child(Leaf leaf) const
    {
        return m_children[leaf];
    }

    //--------------------------------------

    constexpr bool isRed() const { return getColor() == Red; }
    constexpr bool isBlack() const { return getColor() == Black; }
    void setRed() { setColor(Red); }
    void setBlack() { setColor(Black); }

    constexpr CIntrusiveTreeNode*& left() { return child(Left); }
    constexpr CIntrusiveTreeNode*& right() { return child(Right); }
    constexpr CIntrusiveTreeNode* const& left() const { return child(Left); }
    constexpr CIntrusiveTreeNode* const& right() const { return child(Right); }
};

NX_CONSTEXPR CIntrusiveTreeNode::Leaf operator!(CIntrusiveTreeNode::Leaf val) noexcept
{
    return static_cast<CIntrusiveTreeNode::Leaf>(!static_cast<unsigned>(val));
}

class CIntrusiveTreeBase
{
    using N = CIntrusiveTreeNode;

    void rotate(N* node, N::Leaf leaf);
    void recolor(N* parent, N* node);
protected:
    N* m_root;

    constexpr CIntrusiveTreeBase() : m_root{} { }

    N* walk(N* node, N::Leaf leaf) const;
    void insert(N* node, N* parent);
    void remove(N* node);

    N* minmax(N::Leaf leaf) const
    {
        N* p = m_root;
        if (!p)
            return nullptr;
        while (p->child(leaf))
            p = p->child(leaf);
        return p;
    }

    template <typename H>
    N*& navigate(N*& node, N*& parent, N::Leaf leafOnEqual, H helm) const
    {
        node   = nullptr;
        parent = nullptr;

        N** point = const_cast<N**>(&m_root);
        while (*point)
        {
            int direction = helm(*point);
            parent = *point;
            if (direction < 0)
                point = &(*point)->left();
            else if (direction > 0)
                point = &(*point)->right();
            else
            {
                node = *point;
                point = &(*point)->child(leafOnEqual);
            }
        }
        return *point;
    }
};

template <typename ClassT, typename MemberT>
constexpr ClassT* parent_obj(MemberT* member, MemberT ClassT::* ptr)
{
    union whatever
    {
        MemberT ClassT::* ptr;
        intptr_t offset;
    };
    // This is technically UB, but basically every compiler worth using admits it as an extension
    return (ClassT*)((intptr_t)member - whatever{ptr}.offset);
}

template <
    typename T,
    CIntrusiveTreeNode T::* node_ptr,
    typename Comparator = std::less<>
>
class CIntrusiveTree final : protected CIntrusiveTreeBase
{
    using N = CIntrusiveTreeNode;

    static constexpr T* toType(N* m)
    {
        return m ? parent_obj(m, node_ptr) : nullptr;
    }

    static constexpr N* toNode(T* m)
    {
        return m ? &(m->*node_ptr) : nullptr;
    }

    template <typename A, typename B>
    static int compare(A const& a, B const& b)
    {
        Comparator comp;
        if (comp(a, b))
            return -1;
        if (comp(b, a))
            return 1;
        return 0;
    }

public:
    constexpr CIntrusiveTree() : CIntrusiveTreeBase{} { }

    T*     first() const { return toType(minmax(N::Left));  }
    T*     last()  const { return toType(minmax(N::Right)); }
    bool   empty() const { return m_root != nullptr; }
    void   clear()       { m_root = nullptr; }

    T* prev(T* node) const { return toType(walk(toNode(node), N::Left));  }
    T* next(T* node) const { return toType(walk(toNode(node), N::Right)); }

    enum SearchMode
    {
        Exact      = 0,
        LowerBound = 1,
        UpperBound = 2,
    };

    template <typename Lambda>
    T* search(SearchMode mode, Lambda lambda) const
    {
        N *node, *parent;
        N*& point = navigate(node, parent,
            mode != UpperBound ? N::Left : N::Right,
            [&lambda](N* curnode) { return lambda(toType(curnode)); });

        switch (mode)
        {
            default:
            case Exact:
                break;
            case LowerBound:
                if (!node && parent)
                {
                    if (&parent->left() == &point)
                        node = parent;
                    else
                        node = walk(parent, N::Right);
                }
                break;
            case UpperBound:
                if (node)
                    node = walk(node, N::Right);
                else if (parent)
                {
                    if (&parent->right() == &point)
                        node = walk(parent, N::Right);
                    else
                        node = parent;
                }
                break;
        }
        return toType(node);
    }

    template <typename K>
    T* find(K const& key, SearchMode mode = Exact) const
    {
        return search(mode, [&key](T* obj) { return compare(key, *obj); });
    }

    T* insert(T* obj, bool allow_dupes = false)
    {
        N *node, *parent;
        N*& point = navigate(node, parent, N::Right,
            [obj](N* curnode) { return compare(*obj, *toType(curnode)); });

        if (node && !allow_dupes)
            return toType(node);

        point = toNode(obj);
        CIntrusiveTreeBase::insert(point, parent);
        return obj;
    }

    void remove(T* obj)
    {
        CIntrusiveTreeBase::remove(toNode(obj));
    }
};
