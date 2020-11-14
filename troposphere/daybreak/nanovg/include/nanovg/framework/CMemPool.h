/*
** Sample Framework for deko3d Applications
**   CMemPool.h: Pooled dynamic memory allocation manager class
*/
#pragma once
#include "common.h"
#include "CIntrusiveList.h"
#include "CIntrusiveTree.h"

class CMemPool
{
    dk::Device m_dev;
    uint32_t m_flags;
    uint32_t m_blockSize;

    struct Block
    {
        CIntrusiveListNode<Block> m_node;
        dk::MemBlock m_obj;
        void* m_cpuAddr;
        DkGpuAddr m_gpuAddr;

        constexpr void* cpuOffset(uint32_t offset) const
        {
            return m_cpuAddr ? ((u8*)m_cpuAddr + offset) : nullptr;
        }

        constexpr DkGpuAddr gpuOffset(uint32_t offset) const
        {
            return m_gpuAddr != DK_GPU_ADDR_INVALID ? (m_gpuAddr + offset) : DK_GPU_ADDR_INVALID;
        }
    };

    CIntrusiveList<Block, &Block::m_node> m_blocks;

    struct Slice
    {
        CIntrusiveListNode<Slice> m_node;
        CIntrusiveTreeNode m_treenode;
        CMemPool* m_pool;
        Block* m_block;
        uint32_t m_start;
        uint32_t m_end;

        constexpr uint32_t getSize() const { return m_end - m_start; }
        constexpr bool canCoalesce(Slice const& rhs) const { return m_pool == rhs.m_pool && m_block == rhs.m_block && m_end == rhs.m_start; }

        constexpr bool operator<(Slice const& rhs) const { return getSize() < rhs.getSize(); }
        constexpr bool operator<(uint32_t rhs) const { return getSize() < rhs; }
    };

    friend constexpr bool operator<(uint32_t lhs, Slice const& rhs);

    CIntrusiveList<Slice, &Slice::m_node> m_memMap, m_sliceHeap;
    CIntrusiveTree<Slice, &Slice::m_treenode> m_freeList;

    Slice* _newSlice();
    void _deleteSlice(Slice*);

    void _destroy(Slice* slice);

public:
    static constexpr uint32_t DefaultBlockSize = 0x800000;
    class Handle
    {
        Slice* m_slice;
    public:
        constexpr Handle(Slice* slice = nullptr) : m_slice{slice} { }
        constexpr operator bool() const { return m_slice != nullptr; }
        constexpr operator Slice*() const { return m_slice; }
        constexpr bool operator!() const { return !m_slice; }
        constexpr bool operator==(Handle const& rhs) const { return m_slice == rhs.m_slice; }
        constexpr bool operator!=(Handle const& rhs) const { return m_slice != rhs.m_slice; }

        void destroy()
        {
            if (m_slice)
            {
                m_slice->m_pool->_destroy(m_slice);
                m_slice = nullptr;
            }
        }

        constexpr dk::MemBlock getMemBlock() const
        {
            return m_slice->m_block->m_obj;
        }

        constexpr uint32_t getOffset() const
        {
            return m_slice->m_start;
        }

        constexpr uint32_t getSize() const
        {
            return m_slice->getSize();
        }

        constexpr void* getCpuAddr() const
        {
            return m_slice->m_block->cpuOffset(m_slice->m_start);
        }

        constexpr DkGpuAddr getGpuAddr() const
        {
            return m_slice->m_block->gpuOffset(m_slice->m_start);
        }
    };

    CMemPool(dk::Device dev, uint32_t flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, uint32_t blockSize = DefaultBlockSize) :
        m_dev{dev}, m_flags{flags}, m_blockSize{blockSize}, m_blocks{}, m_memMap{}, m_sliceHeap{}, m_freeList{} { }
    ~CMemPool();

    Handle allocate(uint32_t size, uint32_t alignment = DK_CMDMEM_ALIGNMENT);
};

constexpr bool operator<(uint32_t lhs, CMemPool::Slice const& rhs)
{
    return lhs < rhs.getSize();
}
