/*
** Sample Framework for deko3d Applications
**   CExternalImage.cpp: Utility class for loading images from the filesystem
*/
#include "CExternalImage.h"
#include "FileLoader.h"

bool CExternalImage::load(CMemPool& imagePool, CMemPool& scratchPool, dk::Device device, dk::Queue transferQueue, const char* path, uint32_t width, uint32_t height, DkImageFormat format, uint32_t flags)
{
    CMemPool::Handle tempimgmem = LoadFile(scratchPool, path, DK_IMAGE_LINEAR_STRIDE_ALIGNMENT);
    if (!tempimgmem)
        return false;

    dk::UniqueCmdBuf tempcmdbuf = dk::CmdBufMaker{device}.create();
    CMemPool::Handle tempcmdmem = scratchPool.allocate(DK_MEMBLOCK_ALIGNMENT);
    tempcmdbuf.addMemory(tempcmdmem.getMemBlock(), tempcmdmem.getOffset(), tempcmdmem.getSize());

    dk::ImageLayout layout;
    dk::ImageLayoutMaker{device}
        .setFlags(flags)
        .setFormat(format)
        .setDimensions(width, height)
        .initialize(layout);

    m_mem = imagePool.allocate(layout.getSize(), layout.getAlignment());
    m_image.initialize(layout, m_mem.getMemBlock(), m_mem.getOffset());
    m_descriptor.initialize(m_image);

    dk::ImageView imageView{m_image};
    tempcmdbuf.copyBufferToImage({ tempimgmem.getGpuAddr() }, imageView, { 0, 0, 0, width, height, 1 });
    transferQueue.submitCommands(tempcmdbuf.finishList());
    transferQueue.waitIdle();

    tempcmdmem.destroy();
    tempimgmem.destroy();
    return true;
}
