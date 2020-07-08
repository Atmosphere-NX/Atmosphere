#include "dk_renderer.hpp"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <switch.h>

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES /* Enforces GLSL std140/std430 alignment rules for glm types. */
#define GLM_FORCE_INTRINSICS               /* Enables usage of SIMD CPU instructions (requiring the above as well). */
#include <glm/vec2.hpp>

namespace nvg {

    namespace {

        constexpr std::array VertexBufferState = { DkVtxBufferState{sizeof(NVGvertex), 0}, };

        constexpr std::array VertexAttribState = {
            DkVtxAttribState{0, 0, offsetof(NVGvertex, x), DkVtxAttribSize_2x32, DkVtxAttribType_Float, 0},
            DkVtxAttribState{0, 0, offsetof(NVGvertex, u), DkVtxAttribSize_2x32, DkVtxAttribType_Float, 0},
        };

        struct View {
            glm::vec2 size;
        };

        void UpdateImage(dk::Image &image, CMemPool &scratchPool, dk::Device device, dk::Queue transferQueue, int type, int x, int y, int w, int h, const u8 *data) {
            /* Do not proceed if no data is provided upfront. */
            if (data == nullptr) {
                return;
            }

            /* Allocate memory from the pool for the image. */
            const size_t imageSize = type == NVG_TEXTURE_RGBA ? w * h * 4 : w * h;
            CMemPool::Handle tempimgmem = scratchPool.allocate(imageSize, DK_IMAGE_LINEAR_STRIDE_ALIGNMENT);
            memcpy(tempimgmem.getCpuAddr(), data, imageSize);

            dk::UniqueCmdBuf tempcmdbuf = dk::CmdBufMaker{device}.create();
            CMemPool::Handle tempcmdmem = scratchPool.allocate(DK_MEMBLOCK_ALIGNMENT);
            tempcmdbuf.addMemory(tempcmdmem.getMemBlock(), tempcmdmem.getOffset(), tempcmdmem.getSize());

            dk::ImageView imageView{image};
            tempcmdbuf.copyBufferToImage({ tempimgmem.getGpuAddr() }, imageView, { static_cast<uint32_t>(x), static_cast<uint32_t>(y), 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 });

            transferQueue.submitCommands(tempcmdbuf.finishList());
            transferQueue.waitIdle();

            /* Destroy temp mem. */
            tempcmdmem.destroy();
            tempimgmem.destroy();
        }

    }

    Texture::Texture(int id) : m_id(id) { /* ... */ }

    Texture::~Texture() {
        m_image_mem.destroy();
    }

    void Texture::Initialize(CMemPool &image_pool, CMemPool &scratch_pool, dk::Device device, dk::Queue queue, int type, int w, int h, int image_flags, const u8 *data) {
        m_texture_descriptor = {
            .width = w,
            .height = h,
            .type = type,
            .flags = image_flags,
        };

        /* Create an image layout. */
        dk::ImageLayout layout;
        auto layout_maker = dk::ImageLayoutMaker{device}.setFlags(0).setDimensions(w, h);
        if (type == NVG_TEXTURE_RGBA) {
            layout_maker.setFormat(DkImageFormat_RGBA8_Unorm);
        } else {
            layout_maker.setFormat(DkImageFormat_R8_Unorm);
        }
        layout_maker.initialize(layout);

        /* Initialize image. */
        m_image_mem = image_pool.allocate(layout.getSize(), layout.getAlignment());
        m_image.initialize(layout, m_image_mem.getMemBlock(), m_image_mem.getOffset());
        m_image_descriptor.initialize(m_image);

        /* Only update the image if the data isn't null. */
        if (data != nullptr) {
            UpdateImage(m_image, scratch_pool, device, queue, type, 0, 0, w, h, data);
        }
    }

    int Texture::GetId() {
        return m_id;
    }

    const DKNVGtextureDescriptor &Texture::GetDescriptor() {
        return m_texture_descriptor;
    }

    dk::Image &Texture::GetImage() {
        return m_image;
    }

    dk::ImageDescriptor &Texture::GetImageDescriptor() {
        return m_image_descriptor;
    }

    DkRenderer::DkRenderer(unsigned int view_width, unsigned int view_height, dk::Device device, dk::Queue queue, CMemPool &image_mem_pool, CMemPool &code_mem_pool, CMemPool &data_mem_pool) :
        m_view_width(view_width), m_view_height(view_height), m_device(device), m_queue(queue), m_image_mem_pool(image_mem_pool), m_code_mem_pool(code_mem_pool), m_data_mem_pool(data_mem_pool), m_image_descriptor_mappings({0})
    {
        /* Create a dynamic command buffer and allocate memory for it. */
        m_dyn_cmd_buf = dk::CmdBufMaker{m_device}.create();
        m_dyn_cmd_mem.allocate(m_data_mem_pool, DynamicCmdSize);

        m_image_descriptor_set.allocate(m_data_mem_pool);
        m_sampler_descriptor_set.allocate(m_data_mem_pool);

        m_view_uniform_buffer = m_data_mem_pool.allocate(sizeof(View), DK_UNIFORM_BUF_ALIGNMENT);
        m_frag_uniform_buffer = m_data_mem_pool.allocate(sizeof(FragmentUniformSize), DK_UNIFORM_BUF_ALIGNMENT);

        /* Create and bind preset samplers. */
        dk::UniqueCmdBuf init_cmd_buf = dk::CmdBufMaker{m_device}.create();
        CMemPool::Handle init_cmd_mem = m_data_mem_pool.allocate(DK_MEMBLOCK_ALIGNMENT);
        init_cmd_buf.addMemory(init_cmd_mem.getMemBlock(), init_cmd_mem.getOffset(), init_cmd_mem.getSize());

        for (u8 i = 0; i < SamplerType_Total; i++) {
            const DkFilter filter = (i & SamplerType_Nearest) ? DkFilter_Nearest : DkFilter_Linear;
            const DkMipFilter mip_filter = (i & SamplerType_Nearest) ? DkMipFilter_Nearest : DkMipFilter_Linear;
            const DkWrapMode u_wrap_mode = (i & SamplerType_RepeatX) ? DkWrapMode_Repeat : DkWrapMode_ClampToEdge;
            const DkWrapMode v_wrap_mode = (i & SamplerType_RepeatY) ? DkWrapMode_Repeat : DkWrapMode_ClampToEdge;

            auto sampler = dk::Sampler{};
            auto sampler_descriptor = dk::SamplerDescriptor{};
            sampler.setFilter(filter, filter, (i & SamplerType_MipFilter) ? mip_filter : DkMipFilter_None);
            sampler.setWrapMode(u_wrap_mode, v_wrap_mode);
            sampler_descriptor.initialize(sampler);
            m_sampler_descriptor_set.update(init_cmd_buf, i, sampler_descriptor);
        }

        /* Flush the descriptor cache. */
        init_cmd_buf.barrier(DkBarrier_None, DkInvalidateFlags_Descriptors);

        m_sampler_descriptor_set.bindForSamplers(init_cmd_buf);
        m_image_descriptor_set.bindForImages(init_cmd_buf);

        m_queue.submitCommands(init_cmd_buf.finishList());
        m_queue.waitIdle();

        init_cmd_mem.destroy();
        init_cmd_buf.destroy();
    }

    DkRenderer::~DkRenderer() {
        if (m_vertex_buffer) {
            m_vertex_buffer->destroy();
        }

        m_view_uniform_buffer.destroy();
        m_frag_uniform_buffer.destroy();
        m_textures.clear();
    }

    int DkRenderer::AcquireImageDescriptor(std::shared_ptr<Texture> texture, int image) {
        int free_image_descriptor = m_last_image_descriptor + 1;
        int mapping = 0;

        for (int desc = 0; desc <= m_last_image_descriptor; desc++) {
            mapping = m_image_descriptor_mappings[desc];

            /* We've found the image descriptor requested. */
            if (mapping == image) {
                return desc;
            }

            /* Update the free image descriptor. */
            if (mapping == 0 && free_image_descriptor == m_last_image_descriptor + 1) {
                free_image_descriptor = desc;
            }
        }

        /* No descriptors are free. */
        if (free_image_descriptor >= static_cast<int>(MaxImages)) {
            return -1;
        }

        /* Update descriptor sets. */
        m_image_descriptor_set.update(m_dyn_cmd_buf, free_image_descriptor, texture->GetImageDescriptor());

        /* Flush the descriptor cache. */
        m_dyn_cmd_buf.barrier(DkBarrier_None, DkInvalidateFlags_Descriptors);

        /* Update the map. */
        m_image_descriptor_mappings[free_image_descriptor] = image;
        m_last_image_descriptor = free_image_descriptor;
        return free_image_descriptor;
    }

    void DkRenderer::FreeImageDescriptor(int image) {
        for (int desc = 0; desc <= m_last_image_descriptor; desc++) {
            if (m_image_descriptor_mappings[desc] == image) {
                m_image_descriptor_mappings[desc] = 0;
            }
        }
    }

    void DkRenderer::UpdateVertexBuffer(const void *data, size_t size) {
        /* Destroy the existing vertex buffer if it is too small. */
        if (m_vertex_buffer && m_vertex_buffer->getSize() < size) {
            m_vertex_buffer->destroy();
            m_vertex_buffer.reset();
        }

        /* Create a new buffer if needed. */
        if (!m_vertex_buffer) {
            m_vertex_buffer = m_data_mem_pool.allocate(size);
        }

        /* Copy data to the vertex buffer if it exists. */
        if (m_vertex_buffer) {
            memcpy(m_vertex_buffer->getCpuAddr(), data, size);
        }
    }

    void DkRenderer::SetUniforms(const DKNVGcontext &ctx, int offset, int image) {
        m_dyn_cmd_buf.pushConstants(m_frag_uniform_buffer.getGpuAddr(), m_frag_uniform_buffer.getSize(), 0, ctx.fragSize, ctx.uniforms + offset);
        m_dyn_cmd_buf.bindUniformBuffer(DkStage_Fragment, 0, m_frag_uniform_buffer.getGpuAddr(), m_frag_uniform_buffer.getSize());

        /* Attempt to find a texture. */
        const auto texture = this->FindTexture(image);
        if (texture == nullptr) {
            return;
        }

        /* Acquire an image descriptor. */
        const int image_desc_id = this->AcquireImageDescriptor(texture, image);
        if (image_desc_id == -1) {
            return;
        }

        const int image_flags = texture->GetDescriptor().flags;
        uint32_t sampler_id = 0;

        if (image_flags & NVG_IMAGE_GENERATE_MIPMAPS) sampler_id |= SamplerType_MipFilter;
        if (image_flags & NVG_IMAGE_NEAREST)          sampler_id |= SamplerType_Nearest;
        if (image_flags & NVG_IMAGE_REPEATX)          sampler_id |= SamplerType_RepeatX;
        if (image_flags & NVG_IMAGE_REPEATY)          sampler_id |= SamplerType_RepeatY;

        m_dyn_cmd_buf.bindTextures(DkStage_Fragment, 0, dkMakeTextureHandle(image_desc_id, sampler_id));
    }

    void DkRenderer::DrawFill(const DKNVGcontext &ctx, const DKNVGcall &call) {
        DKNVGpath *paths = &ctx.paths[call.pathOffset];
        int npaths = call.pathCount;

        /* Set the stencils to be used. */
        m_dyn_cmd_buf.setStencil(DkFace_FrontAndBack, 0xFF, 0x0, 0xFF);

        /* Set the depth stencil state. */
        auto depth_stencil_state = dk::DepthStencilState{}
            .setStencilTestEnable(true)
            .setStencilFrontCompareOp(DkCompareOp_Always)
            .setStencilFrontFailOp(DkStencilOp_Keep)
            .setStencilFrontDepthFailOp(DkStencilOp_Keep)
            .setStencilFrontPassOp(DkStencilOp_IncrWrap)
            .setStencilBackCompareOp(DkCompareOp_Always)
            .setStencilBackFailOp(DkStencilOp_Keep)
            .setStencilBackDepthFailOp(DkStencilOp_Keep)
            .setStencilBackPassOp(DkStencilOp_DecrWrap);
        m_dyn_cmd_buf.bindDepthStencilState(depth_stencil_state);

        /* Configure for shape drawing. */
        m_dyn_cmd_buf.bindColorWriteState(dk::ColorWriteState{}.setMask(0, 0));
        this->SetUniforms(ctx, call.uniformOffset, 0);
        m_dyn_cmd_buf.bindRasterizerState(dk::RasterizerState{}.setCullMode(DkFace_None));

        /* Draw vertices. */
        for (int i = 0; i < npaths; i++) {
            m_dyn_cmd_buf.draw(DkPrimitive_TriangleFan, paths[i].fillCount, 1, paths[i].fillOffset, 0);
        }

        m_dyn_cmd_buf.bindColorWriteState(dk::ColorWriteState{});
        this->SetUniforms(ctx, call.uniformOffset + ctx.fragSize, call.image);
        m_dyn_cmd_buf.bindRasterizerState(dk::RasterizerState{});

        if (ctx.flags & NVG_ANTIALIAS) {
            /* Configure stencil anti-aliasing. */
            depth_stencil_state
                .setStencilFrontCompareOp(DkCompareOp_Equal)
                .setStencilFrontFailOp(DkStencilOp_Keep)
                .setStencilFrontDepthFailOp(DkStencilOp_Keep)
                .setStencilFrontPassOp(DkStencilOp_Keep)
                .setStencilBackCompareOp(DkCompareOp_Equal)
                .setStencilBackFailOp(DkStencilOp_Keep)
                .setStencilBackDepthFailOp(DkStencilOp_Keep)
                .setStencilBackPassOp(DkStencilOp_Keep);
            m_dyn_cmd_buf.bindDepthStencilState(depth_stencil_state);

            /* Draw fringes. */
            for (int i = 0; i < npaths; i++) {
                m_dyn_cmd_buf.draw(DkPrimitive_TriangleStrip, paths[i].strokeCount, 1, paths[i].strokeOffset, 0);
            }
        }

        /* Configure and draw fill. */
        depth_stencil_state
            .setStencilFrontCompareOp(DkCompareOp_NotEqual)
            .setStencilFrontFailOp(DkStencilOp_Zero)
            .setStencilFrontDepthFailOp(DkStencilOp_Zero)
            .setStencilFrontPassOp(DkStencilOp_Zero)
            .setStencilBackCompareOp(DkCompareOp_NotEqual)
            .setStencilBackFailOp(DkStencilOp_Zero)
            .setStencilBackDepthFailOp(DkStencilOp_Zero)
            .setStencilBackPassOp(DkStencilOp_Zero);
        m_dyn_cmd_buf.bindDepthStencilState(depth_stencil_state);

        m_dyn_cmd_buf.draw(DkPrimitive_TriangleStrip, call.triangleCount, 1, call.triangleOffset, 0);

        /* Reset the depth stencil state to default. */
        m_dyn_cmd_buf.bindDepthStencilState(dk::DepthStencilState{});
    }

    void DkRenderer::DrawConvexFill(const DKNVGcontext &ctx, const DKNVGcall &call) {
        DKNVGpath *paths = &ctx.paths[call.pathOffset];
        int npaths = call.pathCount;

        this->SetUniforms(ctx, call.uniformOffset, call.image);

        for (int i = 0; i < npaths; i++) {
            m_dyn_cmd_buf.draw(DkPrimitive_TriangleFan, paths[i].fillCount, 1, paths[i].fillOffset, 0);

            /* Draw fringes. */
            if (paths[i].strokeCount > 0) {
                m_dyn_cmd_buf.draw(DkPrimitive_TriangleStrip, paths[i].strokeCount, 1, paths[i].strokeOffset, 0);
            }
        }
    }

    void DkRenderer::DrawStroke(const DKNVGcontext &ctx, const DKNVGcall &call) {
        DKNVGpath* paths = &ctx.paths[call.pathOffset];
        int npaths = call.pathCount;

        if (ctx.flags & NVG_STENCIL_STROKES) {
            /* Set the stencil to be used. */
            m_dyn_cmd_buf.setStencil(DkFace_Front, 0xFF, 0x0, 0xFF);

            /* Configure for filling the stroke base without overlap. */
            auto depth_stencil_state = dk::DepthStencilState{}
                .setStencilTestEnable(true)
                .setStencilFrontCompareOp(DkCompareOp_Equal)
                .setStencilFrontFailOp(DkStencilOp_Keep)
                .setStencilFrontDepthFailOp(DkStencilOp_Keep)
                .setStencilFrontPassOp(DkStencilOp_Incr);
            m_dyn_cmd_buf.bindDepthStencilState(depth_stencil_state);
            this->SetUniforms(ctx, call.uniformOffset + ctx.fragSize, call.image);

            /* Draw vertices. */
            for (int i = 0; i < npaths; i++) {
                m_dyn_cmd_buf.draw(DkPrimitive_TriangleStrip, paths[i].strokeCount, 1, paths[i].strokeOffset, 0);
            }

            /* Configure for drawing anti-aliased pixels. */
            depth_stencil_state.setStencilFrontPassOp(DkStencilOp_Keep);
            m_dyn_cmd_buf.bindDepthStencilState(depth_stencil_state);
            this->SetUniforms(ctx, call.uniformOffset, call.image);

            /* Draw vertices. */
            for (int i = 0; i < npaths; i++) {
                m_dyn_cmd_buf.draw(DkPrimitive_TriangleStrip, paths[i].strokeCount, 1, paths[i].strokeOffset, 0);
            }

            /* Configure for clearing the stencil buffer. */
            depth_stencil_state
                .setStencilTestEnable(true)
                .setStencilFrontCompareOp(DkCompareOp_Always)
                .setStencilFrontFailOp(DkStencilOp_Zero)
                .setStencilFrontDepthFailOp(DkStencilOp_Zero)
                .setStencilFrontPassOp(DkStencilOp_Zero);
            m_dyn_cmd_buf.bindDepthStencilState(depth_stencil_state);

            /* Draw vertices. */
            for (int i = 0; i < npaths; i++) {
                m_dyn_cmd_buf.draw(DkPrimitive_TriangleStrip, paths[i].strokeCount, 1, paths[i].strokeOffset, 0);
            }

            /* Reset the depth stencil state to default. */
            m_dyn_cmd_buf.bindDepthStencilState(dk::DepthStencilState{});
        } else {
            this->SetUniforms(ctx, call.uniformOffset, call.image);

            /* Draw vertices. */
            for (int i = 0; i < npaths; i++) {
                m_dyn_cmd_buf.draw(DkPrimitive_TriangleStrip, paths[i].strokeCount, 1, paths[i].strokeOffset, 0);
            }
        }
    }

    void DkRenderer::DrawTriangles(const DKNVGcontext &ctx, const DKNVGcall &call) {
        this->SetUniforms(ctx, call.uniformOffset, call.image);
        m_dyn_cmd_buf.draw(DkPrimitive_Triangles, call.triangleCount, 1, call.triangleOffset, 0);
    }

    int DkRenderer::Create(DKNVGcontext &ctx) {
        m_vertex_shader.load(m_code_mem_pool, "romfs:/shaders/fill_vsh.dksh");

        /* Load the appropriate fragment shader depending on whether AA is enabled. */
        if (ctx.flags & NVG_ANTIALIAS) {
            m_fragment_shader.load(m_code_mem_pool, "romfs:/shaders/fill_aa_fsh.dksh");
        } else {
            m_fragment_shader.load(m_code_mem_pool, "romfs:/shaders/fill_fsh.dksh");
        }

        /* Set the size of fragment uniforms. */
        ctx.fragSize = FragmentUniformSize;
        return 1;
    }

    std::shared_ptr<Texture> DkRenderer::FindTexture(int id) {
        for (auto it = m_textures.begin(); it != m_textures.end(); it++) {
            if ((*it)->GetId() == id) {
                return *it;
            }
        }

        return nullptr;
    }

    int DkRenderer::CreateTexture(const DKNVGcontext &ctx, int type, int w, int h, int image_flags, const unsigned char* data) {
        const auto texture_id = m_next_texture_id++;
        auto texture = std::make_shared<Texture>(texture_id);
        texture->Initialize(m_image_mem_pool, m_data_mem_pool, m_device, m_queue, type, w, h, image_flags, data);
        m_textures.push_back(texture);
        return texture->GetId();
    }

    int DkRenderer::DeleteTexture(const DKNVGcontext &ctx, int image) {
        bool found = false;

        for (auto it = m_textures.begin(); it != m_textures.end();) {
            /* Remove textures with the given id. */
            if ((*it)->GetId() == image) {
                it = m_textures.erase(it);
                found = true;
            } else {
                ++it;
            }
        }

        /* Free any used image descriptors. */
        this->FreeImageDescriptor(image);
        return found;
    }

    int DkRenderer::UpdateTexture(const DKNVGcontext &ctx, int image, int x, int y, int w, int h, const unsigned char *data) {
        const std::shared_ptr<Texture> texture = this->FindTexture(image);

        /* Could not find a texture. */
        if (texture == nullptr) {
            return 0;
        }

        const DKNVGtextureDescriptor &tex_desc = texture->GetDescriptor();
        if (tex_desc.type == NVG_TEXTURE_RGBA) {
            data += y * tex_desc.width*4;
        } else {
            data += y * tex_desc.width;
        }
        x = 0;
        w = tex_desc.width;

        UpdateImage(texture->GetImage(), m_data_mem_pool, m_device, m_queue, tex_desc.type, x, y, w, h, data);
        return 1;
    }

    int DkRenderer::GetTextureSize(const DKNVGcontext &ctx, int image, int *w, int *h) {
        const auto descriptor = this->GetTextureDescriptor(ctx, image);
        if (descriptor == nullptr) {
            return 0;
        }

        *w = descriptor->width;
        *h = descriptor->height;
        return 1;
    }

    const DKNVGtextureDescriptor *DkRenderer::GetTextureDescriptor(const DKNVGcontext &ctx, int id) {
        for (auto it = m_textures.begin(); it != m_textures.end(); it++) {
            if ((*it)->GetId() == id) {
                return &(*it)->GetDescriptor();
            }
        }

        return nullptr;
    }

    void DkRenderer::Flush(DKNVGcontext &ctx) {
        if (ctx.ncalls > 0) {
            /* Prepare dynamic command buffer. */
            m_dyn_cmd_mem.begin(m_dyn_cmd_buf);

            /* Update buffers with data. */
            this->UpdateVertexBuffer(ctx.verts, ctx.nverts * sizeof(NVGvertex));

            /* Enable blending. */
            m_dyn_cmd_buf.bindColorState(dk::ColorState{}.setBlendEnable(0, true));

            /* Setup. */
            m_dyn_cmd_buf.bindShaders(DkStageFlag_GraphicsMask, { m_vertex_shader, m_fragment_shader });
            m_dyn_cmd_buf.bindVtxAttribState(VertexAttribState);
            m_dyn_cmd_buf.bindVtxBufferState(VertexBufferState);
            m_dyn_cmd_buf.bindVtxBuffer(0, m_vertex_buffer->getGpuAddr(), m_vertex_buffer->getSize());

            /* Push the view size to the uniform buffer and bind it. */
            const auto view = View{glm::vec2{m_view_width, m_view_height}};
            m_dyn_cmd_buf.pushConstants(m_view_uniform_buffer.getGpuAddr(), m_view_uniform_buffer.getSize(), 0, sizeof(view), &view);
            m_dyn_cmd_buf.bindUniformBuffer(DkStage_Vertex, 0, m_view_uniform_buffer.getGpuAddr(), m_view_uniform_buffer.getSize());

            /* Iterate over calls. */
            for (int i = 0; i < ctx.ncalls; i++) {
                const DKNVGcall &call = ctx.calls[i];

                /* Perform blending. */
                m_dyn_cmd_buf.bindBlendStates(0, { dk::BlendState{}.setFactors(static_cast<DkBlendFactor>(call.blendFunc.srcRGB), static_cast<DkBlendFactor>(call.blendFunc.dstRGB), static_cast<DkBlendFactor>(call.blendFunc.srcAlpha), static_cast<DkBlendFactor>(call.blendFunc.dstRGB)) });

                if (call.type == DKNVG_FILL) {
                    this->DrawFill(ctx, call);
                } else if (call.type == DKNVG_CONVEXFILL) {
                    this->DrawConvexFill(ctx, call);
                } else if (call.type == DKNVG_STROKE) {
                    this->DrawStroke(ctx, call);
                } else if (call.type == DKNVG_TRIANGLES) {
                    this->DrawTriangles(ctx, call);
                }
            }

            m_queue.submitCommands(m_dyn_cmd_mem.end(m_dyn_cmd_buf));
        }

        /* Reset calls. */
        ctx.nverts = 0;
        ctx.npaths = 0;
        ctx.ncalls = 0;
        ctx.nuniforms = 0;
    }

}