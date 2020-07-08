/*
** Sample Framework for deko3d Applications
**   CCmdMemRing.h: Memory provider class for dynamic command buffers
*/
#pragma once
#include "common.h"
#include "CMemPool.h"

template <unsigned NumSlices>
class CCmdMemRing
{
    static_assert(NumSlices > 0, "Need a non-zero number of slices...");
    CMemPool::Handle m_mem;
    unsigned m_curSlice;
    dk::Fence m_fences[NumSlices];
public:
    CCmdMemRing() : m_mem{}, m_curSlice{}, m_fences{} { }
    ~CCmdMemRing()
    {
        m_mem.destroy();
    }

    bool allocate(CMemPool& pool, uint32_t sliceSize)
    {
        sliceSize = (sliceSize + DK_CMDMEM_ALIGNMENT - 1) &~ (DK_CMDMEM_ALIGNMENT - 1);
        m_mem = pool.allocate(NumSlices*sliceSize);
        return m_mem;
    }

    void begin(dk::CmdBuf cmdbuf)
    {
        // Clear/reset the command buffer, which also destroys all command list handles
        // (but remember: it does *not* in fact destroy the command data)
        cmdbuf.clear();

        // Wait for the current slice of memory to be available, and feed it to the command buffer
        uint32_t sliceSize = m_mem.getSize() / NumSlices;
        m_fences[m_curSlice].wait();

        // Feed the memory to the command buffer
        cmdbuf.addMemory(m_mem.getMemBlock(), m_mem.getOffset() + m_curSlice * sliceSize, sliceSize);
    }

    DkCmdList end(dk::CmdBuf cmdbuf)
    {
        // Signal the fence corresponding to the current slice; so that in the future when we want
        // to use it again, we can wait for the completion of the commands we've just submitted
        // (and as such we don't overwrite in-flight command data with new one)
        cmdbuf.signalFence(m_fences[m_curSlice]);

        // Advance the current slice counter; wrapping around when we reach the end
        m_curSlice = (m_curSlice + 1) % NumSlices;

        // Finish off the command list, returning it to the caller
        return cmdbuf.finishList();
    }
};
