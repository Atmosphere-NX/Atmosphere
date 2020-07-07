/*
** Sample Framework for deko3d Applications
**   CMemPool.cpp: Pooled dynamic memory allocation manager class
*/
#include "CMemPool.h"

inline auto CMemPool::_newSlice() -> Slice*
{
    Slice* ret = m_sliceHeap.pop();
    if (!ret) ret = (Slice*)::malloc(sizeof(Slice));
    return ret;
}

inline void CMemPool::_deleteSlice(Slice* s)
{
    if (!s) return;
    m_sliceHeap.add(s);
}

CMemPool::~CMemPool()
{
    m_memMap.iterate([](Slice* s) { ::free(s); });
    m_sliceHeap.iterate([](Slice* s) { ::free(s); });
    m_blocks.iterate([](Block* blk) {
        blk->m_obj.destroy();
        ::free(blk);
    });
}

auto CMemPool::allocate(uint32_t size, uint32_t alignment) -> Handle
{
    if (!size) return nullptr;
    if (alignment & (alignment - 1)) return nullptr;
    size = (size + alignment - 1) &~ (alignment - 1);
#ifdef DEBUG_CMEMPOOL
    printf("Allocating size=%u alignment=0x%x\n", size, alignment);
    {
        Slice* temp = /*m_freeList*/m_memMap.first();
        while (temp)
        {
            printf("-- blk %p | 0x%08x-0x%08x | %s used\n", temp->m_block, temp->m_start, temp->m_end, temp->m_pool ? "   " : "not");
            temp = /*m_freeList*/m_memMap.next(temp);
        }
    }
#endif

    uint32_t start_offset = 0;
    uint32_t end_offset = 0;
    Slice* slice = m_freeList.find(size, decltype(m_freeList)::LowerBound);
    while (slice)
    {
#ifdef DEBUG_CMEMPOOL
        printf(" * Checking slice 0x%x - 0x%x\n", slice->m_start, slice->m_end);
#endif
        start_offset = (slice->m_start + alignment - 1) &~ (alignment - 1);
        end_offset = start_offset + size;
        if (end_offset <= slice->m_end)
            break;
        slice = m_freeList.next(slice);
    }

    if (!slice)
    {
        Block* blk = (Block*)::malloc(sizeof(Block));
        if (!blk)
            return nullptr;

        uint32_t unusableSize = (m_flags & DkMemBlockFlags_Code) ? DK_SHADER_CODE_UNUSABLE_SIZE : 0;
        uint32_t blkSize = m_blockSize - unusableSize;
        blkSize = size > blkSize ? size : blkSize;
        blkSize = (blkSize + unusableSize + DK_MEMBLOCK_ALIGNMENT - 1) &~ (DK_MEMBLOCK_ALIGNMENT - 1);
#ifdef DEBUG_CMEMPOOL
        printf(" ! Allocating block of size 0x%x\n", blkSize);
#endif
        blk->m_obj = dk::MemBlockMaker{m_dev, blkSize}.setFlags(m_flags).create();
        if (!blk->m_obj)
        {
            ::free(blk);
            return nullptr;
        }

        slice = _newSlice();
        if (!slice)
        {
            blk->m_obj.destroy();
            ::free(blk);
            return nullptr;
        }

        slice->m_pool = nullptr;
        slice->m_block = blk;
        slice->m_start = 0;
        slice->m_end = blkSize - unusableSize;
        m_memMap.add(slice);

        blk->m_cpuAddr = blk->m_obj.getCpuAddr();
        blk->m_gpuAddr = blk->m_obj.getGpuAddr();
        m_blocks.add(blk);

        start_offset = 0;
        end_offset = size;
    }
    else
    {
#ifdef DEBUG_CMEMPOOL
        printf(" * found it\n");
#endif
        m_freeList.remove(slice);
    }

    if (start_offset != slice->m_start)
    {
        Slice* t = _newSlice();
        if (!t) goto _bad;
        t->m_pool = nullptr;
        t->m_block = slice->m_block;
        t->m_start = slice->m_start;
        t->m_end = start_offset;
#ifdef DEBUG_CMEMPOOL
        printf("-> subdivide left:  %08x-%08x\n", t->m_start, t->m_end);
#endif
        m_memMap.addBefore(slice, t);
        m_freeList.insert(t, true);
        slice->m_start = start_offset;
    }

    if (end_offset != slice->m_end)
    {
        Slice* t = _newSlice();
        if (!t) goto _bad;
        t->m_pool = nullptr;
        t->m_block = slice->m_block;
        t->m_start = end_offset;
        t->m_end = slice->m_end;
#ifdef DEBUG_CMEMPOOL
        printf("-> subdivide right: %08x-%08x\n", t->m_start, t->m_end);
#endif
        m_memMap.addAfter(slice, t);
        m_freeList.insert(t, true);
        slice->m_end = end_offset;
    }

    slice->m_pool = this;
    return slice;

_bad:
    m_freeList.insert(slice, true);
    return nullptr;
}

void CMemPool::_destroy(Slice* slice)
{
    slice->m_pool = nullptr;

    Slice* left  = m_memMap.prev(slice);
    Slice* right = m_memMap.next(slice);

    if (left && left->canCoalesce(*slice))
    {
        slice->m_start = left->m_start;
        m_freeList.remove(left);
        m_memMap.remove(left);
        _deleteSlice(left);
    }

    if (right && slice->canCoalesce(*right))
    {
        slice->m_end = right->m_end;
        m_freeList.remove(right);
        m_memMap.remove(right);
        _deleteSlice(right);
    }

    m_freeList.insert(slice, true);
}
