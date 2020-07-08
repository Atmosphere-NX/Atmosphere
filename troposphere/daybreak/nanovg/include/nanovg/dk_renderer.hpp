#pragma once

#include <deko3d.hpp>
#include <map>
#include <memory>
#include <vector>

#include "framework/CDescriptorSet.h"
#include "framework/CMemPool.h"
#include "framework/CShader.h"
#include "framework/CCmdMemRing.h"
#include "nanovg.h"

// Create flags
enum NVGcreateFlags {
    // Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
    NVG_ANTIALIAS 		= 1<<0,
    // Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
    // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
    NVG_STENCIL_STROKES	= 1<<1,
    // Flag indicating that additional debug checks are done.
    NVG_DEBUG 			= 1<<2,
};

enum DKNVGuniformLoc
{
    DKNVG_LOC_VIEWSIZE,
    DKNVG_LOC_TEX,
    DKNVG_LOC_FRAG,
    DKNVG_MAX_LOCS
};

enum VKNVGshaderType {
  NSVG_SHADER_FILLGRAD,
  NSVG_SHADER_FILLIMG,
  NSVG_SHADER_SIMPLE,
  NSVG_SHADER_IMG
};

struct DKNVGtextureDescriptor {
    int width, height;
    int type;
    int flags;
};

struct DKNVGblend {
    int srcRGB;
    int dstRGB;
    int srcAlpha;
    int dstAlpha;
};

enum DKNVGcallType {
    DKNVG_NONE = 0,
    DKNVG_FILL,
    DKNVG_CONVEXFILL,
    DKNVG_STROKE,
    DKNVG_TRIANGLES,
};

struct DKNVGcall {
    int type;
    int image;
    int pathOffset;
    int pathCount;
    int triangleOffset;
    int triangleCount;
    int uniformOffset;
    DKNVGblend blendFunc;
};

struct DKNVGpath {
    int fillOffset;
    int fillCount;
    int strokeOffset;
    int strokeCount;
};

struct DKNVGfragUniforms {
    float scissorMat[12]; // matrices are actually 3 vec4s
    float paintMat[12];
    struct NVGcolor innerCol;
    struct NVGcolor outerCol;
    float scissorExt[2];
    float scissorScale[2];
    float extent[2];
    float radius;
    float feather;
    float strokeMult;
    float strokeThr;
    int texType;
    int type;
};

namespace nvg {
    class DkRenderer;
}

struct DKNVGcontext {
    nvg::DkRenderer *renderer;
    float view[2];
    int fragSize;
    int flags;
    // Per frame buffers
    DKNVGcall* calls;
    int ccalls;
    int ncalls;
    DKNVGpath* paths;
    int cpaths;
    int npaths;
    struct NVGvertex* verts;
    int cverts;
    int nverts;
    unsigned char* uniforms;
    int cuniforms;
    int nuniforms;
};

namespace nvg {

    class Texture {
        private:
            const int m_id;
            dk::Image m_image;
            dk::ImageDescriptor m_image_descriptor;
            CMemPool::Handle m_image_mem;
            DKNVGtextureDescriptor m_texture_descriptor;
        public:
            Texture(int id);
            ~Texture();

            void Initialize(CMemPool &image_pool, CMemPool &scratch_pool, dk::Device device, dk::Queue transfer_queue, int type, int w, int h, int image_flags, const u8 *data);
            void Update(CMemPool &image_pool, CMemPool &scratch_pool, dk::Device device, dk::Queue transfer_queue, int type, int w, int h, int image_flags, const u8 *data);

            int GetId();
            const DKNVGtextureDescriptor &GetDescriptor();

            dk::Image &GetImage();
            dk::ImageDescriptor &GetImageDescriptor();
    };

    class DkRenderer {
        private:
            enum SamplerType : u8 {
                SamplerType_MipFilter = 1 << 0,
                SamplerType_Nearest   = 1 << 1,
                SamplerType_RepeatX   = 1 << 2,
                SamplerType_RepeatY   = 1 << 3,
                SamplerType_Total     = 0x10,
            };
        private:
            static constexpr size_t DynamicCmdSize = 0x20000;
            static constexpr size_t FragmentUniformSize = sizeof(DKNVGfragUniforms) + 4 - sizeof(DKNVGfragUniforms) % 4;
            static constexpr size_t MaxImages = 0x1000;

            /* From the application. */
            u32 m_view_width;
            u32 m_view_height;
            dk::Device m_device;
            dk::Queue m_queue;
            CMemPool &m_image_mem_pool;
            CMemPool &m_code_mem_pool;
            CMemPool &m_data_mem_pool;

            /* State. */
            dk::UniqueCmdBuf m_dyn_cmd_buf;
            CCmdMemRing<1> m_dyn_cmd_mem;
            std::optional<CMemPool::Handle> m_vertex_buffer;
            CShader m_vertex_shader;
            CShader m_fragment_shader;
            CMemPool::Handle m_view_uniform_buffer;
            CMemPool::Handle m_frag_uniform_buffer;

            u32 m_next_texture_id = 1;
            std::vector<std::shared_ptr<Texture>> m_textures;
            CDescriptorSet<MaxImages> m_image_descriptor_set;
            CDescriptorSet<SamplerType_Total> m_sampler_descriptor_set;
            std::array<int, MaxImages> m_image_descriptor_mappings;
            int m_last_image_descriptor = 0;

            int AcquireImageDescriptor(std::shared_ptr<Texture> texture, int image);
            void FreeImageDescriptor(int image);
            void SetUniforms(const DKNVGcontext &ctx, int offset, int image);

            void UpdateVertexBuffer(const void *data, size_t size);

            void DrawFill(const DKNVGcontext &ctx, const DKNVGcall &call);
            void DrawConvexFill(const DKNVGcontext &ctx, const DKNVGcall &call);
            void DrawStroke(const DKNVGcontext &ctx, const DKNVGcall &call);
            void DrawTriangles(const DKNVGcontext &ctx, const DKNVGcall &call);

            std::shared_ptr<Texture> FindTexture(int id);
        public:
            DkRenderer(unsigned int view_width, unsigned int view_height, dk::Device device, dk::Queue queue, CMemPool &image_mem_pool, CMemPool &code_mem_pool, CMemPool &data_mem_pool);
            ~DkRenderer();

            int Create(DKNVGcontext &ctx);
            int CreateTexture(const DKNVGcontext &ctx, int type, int w, int h, int image_flags, const u8 *data);
            int DeleteTexture(const DKNVGcontext &ctx, int id);
            int UpdateTexture(const DKNVGcontext &ctx, int id, int x, int y, int w, int h, const u8 *data);
            int GetTextureSize(const DKNVGcontext &ctx, int id, int *w, int *h);
            const DKNVGtextureDescriptor *GetTextureDescriptor(const DKNVGcontext &ctx, int id);

            void Flush(DKNVGcontext &ctx);
    };

}