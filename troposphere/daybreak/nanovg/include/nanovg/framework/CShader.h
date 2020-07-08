/*
** Sample Framework for deko3d Applications
**   CShader.h: Utility class for loading shaders from the filesystem
*/
#pragma once
#include "common.h"
#include "CMemPool.h"

class CShader
{
    dk::Shader m_shader;
    CMemPool::Handle m_codemem;
public:
    CShader() : m_shader{}, m_codemem{} { }
    ~CShader()
    {
        m_codemem.destroy();
    }

    constexpr operator bool() const
    {
        return m_codemem;
    }

    constexpr operator dk::Shader const*() const
    {
        return &m_shader;
    }

    bool load(CMemPool& pool, const char* path);
};
