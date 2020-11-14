/*
** Sample Framework for deko3d Applications
**   CIntrusiveList.h: Intrusive doubly-linked list helper class
*/
#pragma once
#include "common.h"

template <typename T>
struct CIntrusiveListNode
{
    T *m_next, *m_prev;

    constexpr CIntrusiveListNode() : m_next{}, m_prev{} { }
    constexpr operator bool() const { return m_next || m_prev; }
};

template <typename T, CIntrusiveListNode<T> T::* node_ptr>
class CIntrusiveList
{
    T *m_first, *m_last;

public:
    constexpr CIntrusiveList() : m_first{}, m_last{} { }

    constexpr T*   first() const { return m_first; }
    constexpr T*   last()  const { return m_last; }
    constexpr bool empty() const { return !m_first; }
    constexpr void clear()       { m_first = m_last = nullptr; }

    constexpr bool isLinked(T* obj) const { return obj->*node_ptr || m_first == obj; }
    constexpr T*   prev(T* obj) const { return (obj->*node_ptr).m_prev; }
    constexpr T*   next(T* obj) const { return (obj->*node_ptr).m_next; }

    void add(T* obj)
    {
        return addBefore(nullptr, obj);
    }

    void addBefore(T* pos, T* obj)
    {
        auto& node = obj->*node_ptr;
        node.m_next = pos;
        node.m_prev = pos ? (pos->*node_ptr).m_prev : m_last;

        if (pos)
            (pos->*node_ptr).m_prev = obj;
        else
            m_last = obj;

        if (node.m_prev)
            (node.m_prev->*node_ptr).m_next = obj;
        else
            m_first = obj;
    }

    void addAfter(T* pos, T* obj)
    {
        auto& node = obj->*node_ptr;
        node.m_next = pos ? (pos->*node_ptr).m_next : m_first;
        node.m_prev = pos;

        if (pos)
            (pos->*node_ptr).m_next = obj;
        else
            m_first = obj;

        if (node.m_next)
            (node.m_next->*node_ptr).m_prev = obj;
        else
            m_last = obj;
    }

    T* pop()
    {
        T* ret = m_first;
        if (ret)
        {
            m_first = (ret->*node_ptr).m_next;
            if (m_first)
                (m_first->*node_ptr).m_prev = nullptr;
            else
                m_last = nullptr;
        }
        return ret;
    }

    void remove(T* obj)
    {
        auto& node = obj->*node_ptr;
        if (node.m_prev)
        {
            (node.m_prev->*node_ptr).m_next = node.m_next;
            if (node.m_next)
                (node.m_next->*node_ptr).m_prev = node.m_prev;
            else
                m_last = node.m_prev;
        } else
        {
            m_first = node.m_next;
            if (m_first)
                (m_first->*node_ptr).m_prev = nullptr;
            else
                m_last = nullptr;
        }

        node.m_next = node.m_prev = 0;
    }

    template <typename L>
    void iterate(L lambda) const
    {
        T* next = nullptr;
        for (T* cur = m_first; cur; cur = next)
        {
            next = (cur->*node_ptr).m_next;
            lambda(cur);
        }
    }
};
