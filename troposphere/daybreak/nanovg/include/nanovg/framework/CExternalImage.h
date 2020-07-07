/*
** Sample Framework for deko3d Applications
**   CExternalImage.h: Utility class for loading images from the filesystem
*/
#pragma once
#include "common.h"
#include "CMemPool.h"

class CExternalImage
{
    dk::Image m_image;
    dk::ImageDescriptor m_descriptor;
    CMemPool::Handle m_mem;
public:
    CExternalImage() : m_image{}, m_descriptor{}, m_mem{} { }
    ~CExternalImage()
    {
        m_mem.destroy();
    }

    constexpr operator bool() const
    {
        return m_mem;
    }

    constexpr dk::Image& get()
    {
        return m_image;
    }

    constexpr dk::ImageDescriptor const& getDescriptor() const
    {
        return m_descriptor;
    }

    bool load(CMemPool& imagePool, CMemPool& scratchPool, dk::Device device, dk::Queue transferQueue, const char* path, uint32_t width, uint32_t height, DkImageFormat format, uint32_t flags = 0);
};
