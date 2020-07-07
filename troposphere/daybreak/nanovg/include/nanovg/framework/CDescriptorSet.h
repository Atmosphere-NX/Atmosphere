/*
** Sample Framework for deko3d Applications
**   CDescriptorSet.h: Image/Sampler descriptor set class
*/
#pragma once
#include "common.h"
#include "CMemPool.h"

template <unsigned NumDescriptors>
class CDescriptorSet
{
    static_assert(NumDescriptors > 0, "Need a non-zero number of descriptors...");
    static_assert(sizeof(DkImageDescriptor) == sizeof(DkSamplerDescriptor), "shouldn't happen");
    static_assert(DK_IMAGE_DESCRIPTOR_ALIGNMENT == DK_SAMPLER_DESCRIPTOR_ALIGNMENT, "shouldn't happen");
    static constexpr size_t DescriptorSize = sizeof(DkImageDescriptor);
    static constexpr size_t DescriptorAlign = DK_IMAGE_DESCRIPTOR_ALIGNMENT;

    CMemPool::Handle m_mem;
public:
    CDescriptorSet() : m_mem{} { }
    ~CDescriptorSet()
    {
        m_mem.destroy();
    }

    bool allocate(CMemPool& pool)
    {
        m_mem = pool.allocate(NumDescriptors*DescriptorSize, DescriptorAlign);
        return m_mem;
    }

    void bindForImages(dk::CmdBuf cmdbuf)
    {
        cmdbuf.bindImageDescriptorSet(m_mem.getGpuAddr(), NumDescriptors);
    }

    void bindForSamplers(dk::CmdBuf cmdbuf)
    {
        cmdbuf.bindSamplerDescriptorSet(m_mem.getGpuAddr(), NumDescriptors);
    }

    template <typename T>
    void update(dk::CmdBuf cmdbuf, uint32_t id, T const& descriptor)
    {
        static_assert(sizeof(T) == DescriptorSize);
        cmdbuf.pushData(m_mem.getGpuAddr() + id*DescriptorSize, &descriptor, DescriptorSize);
    }

    template <typename T, size_t N>
    void update(dk::CmdBuf cmdbuf, uint32_t id, std::array<T, N> const& descriptors)
    {
        static_assert(sizeof(T) == DescriptorSize);
        cmdbuf.pushData(m_mem.getGpuAddr() + id*DescriptorSize, descriptors.data(), descriptors.size()*DescriptorSize);
    }

#ifdef DK_HPP_SUPPORT_VECTOR
    template <typename T, typename Allocator = std::allocator<T>>
    void update(dk::CmdBuf cmdbuf, uint32_t id, std::vector<T,Allocator> const& descriptors)
    {
        static_assert(sizeof(T) == DescriptorSize);
        cmdbuf.pushData(m_mem.getGpuAddr() + id*DescriptorSize, descriptors.data(), descriptors.size()*DescriptorSize);
    }
#endif

    template <typename T>
    void update(dk::CmdBuf cmdbuf, uint32_t id, std::initializer_list<T const> const& descriptors)
    {
        static_assert(sizeof(T) == DescriptorSize);
        cmdbuf.pushData(m_mem.getGpuAddr() + id*DescriptorSize, descriptors.data(), descriptors.size()*DescriptorSize);
    }
};
