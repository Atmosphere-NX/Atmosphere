/*
 * Copyright (c) 2020 Adubbz
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <optional>
#include <switch.h>
#include <nanovg.h>
#include <nanovg_dk.h>
#include <nanovg/framework/CApplication.h>
#include "ui.hpp"
#include "ams_su.h"

extern "C" {

    void userAppInit(void) {
        Result rc = 0;

        if (R_FAILED(rc = romfsInit())) {
            fatalThrow(rc);
        }

        if (R_FAILED(rc = spsmInitialize())) {
            fatalThrow(rc);
        }

        if (R_FAILED(rc = plInitialize(PlServiceType_User))) {
            fatalThrow(rc);
        }

        if (R_FAILED(rc = splInitialize())) {
            fatalThrow(rc);
        }

        if (R_FAILED(rc = nsInitialize())) {
            fatalThrow(rc);
        }

        if (R_FAILED(rc = hiddbgInitialize())) {
            fatalThrow(rc);
        }

    }

    void userAppExit(void) {
        hiddbgExit();
        nsExit();
        splExit();
        plExit();
        spsmExit();
        romfsExit();
        amssuExit();
    }

}

namespace {

    static constexpr u32 FramebufferWidth = 1280;
    static constexpr u32 FramebufferHeight = 720;

}

class Daybreak : public CApplication {
    private:
        static constexpr unsigned NumFramebuffers = 2;
        static constexpr unsigned StaticCmdSize = 0x1000;

        dk::UniqueDevice m_device;
        dk::UniqueQueue m_queue;
        dk::UniqueSwapchain m_swapchain;

        std::optional<CMemPool> m_pool_images;
        std::optional<CMemPool> m_pool_code;
        std::optional<CMemPool> m_pool_data;

        dk::UniqueCmdBuf m_cmd_buf;
        DkCmdList m_render_cmdlist;

        dk::Image m_depth_buffer;
        CMemPool::Handle m_depth_buffer_mem;
        dk::Image m_framebuffers[NumFramebuffers];
        CMemPool::Handle m_framebuffers_mem[NumFramebuffers];
        DkCmdList m_framebuffer_cmdlists[NumFramebuffers];

        std::optional<nvg::DkRenderer> m_renderer;
        NVGcontext *m_vg;
        int m_standard_font;
    public:
        Daybreak() {
            Result rc = 0;

            /* Create the deko3d device. */
            m_device = dk::DeviceMaker{}.create();

            /* Create the main queue. */
            m_queue = dk::QueueMaker{m_device}.setFlags(DkQueueFlags_Graphics).create();

            /* Create the memory pools. */
            m_pool_images.emplace(m_device, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image, 16*1024*1024);
            m_pool_code.emplace(m_device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code, 128*1024);
            m_pool_data.emplace(m_device, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached, 1*1024*1024);

            /* Create the static command buffer and feed it freshly allocated memory. */
            m_cmd_buf = dk::CmdBufMaker{m_device}.create();
            CMemPool::Handle cmdmem = m_pool_data->allocate(StaticCmdSize);
            m_cmd_buf.addMemory(cmdmem.getMemBlock(), cmdmem.getOffset(), cmdmem.getSize());

            /* Create the framebuffer resources. */
            this->CreateFramebufferResources();

            m_renderer.emplace(FramebufferWidth, FramebufferHeight, m_device, m_queue, *m_pool_images, *m_pool_code, *m_pool_data);
            m_vg = nvgCreateDk(&*m_renderer, NVG_ANTIALIAS | NVG_STENCIL_STROKES);


            PlFontData font;
            if (R_FAILED(rc = plGetSharedFontByType(&font, PlSharedFontType_Standard))) {
                fatalThrow(rc);
            }

            m_standard_font = nvgCreateFontMem(m_vg, "switch-standard", static_cast<u8 *>(font.address), font.size, 0);
        }

        ~Daybreak() {
            /* Destroy the framebuffer resources. This should be done first. */
            this->DestroyFramebufferResources();

            /* Cleanup vg. */
            nvgDeleteDk(m_vg);

            /* Destroy the renderer. */
            m_renderer.reset();
        }
    private:
        void CreateFramebufferResources() {
            /* Create layout for the depth buffer. */
            dk::ImageLayout layout_depth_buffer;
            dk::ImageLayoutMaker{m_device}
                .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
                .setFormat(DkImageFormat_S8)
                .setDimensions(FramebufferWidth, FramebufferHeight)
                .initialize(layout_depth_buffer);

            /* Create the depth buffer. */
            m_depth_buffer_mem = m_pool_images->allocate(layout_depth_buffer.getSize(), layout_depth_buffer.getAlignment());
            m_depth_buffer.initialize(layout_depth_buffer, m_depth_buffer_mem.getMemBlock(), m_depth_buffer_mem.getOffset());

            /* Create layout for the framebuffers. */
            dk::ImageLayout layout_framebuffer;
            dk::ImageLayoutMaker{m_device}
                .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
                .setFormat(DkImageFormat_RGBA8_Unorm)
                .setDimensions(FramebufferWidth, FramebufferHeight)
                .initialize(layout_framebuffer);

            /* Create the framebuffers. */
            std::array<DkImage const*, NumFramebuffers> fb_array;
            const u64 fb_size  = layout_framebuffer.getSize();
            const u32 fb_align = layout_framebuffer.getAlignment();

            for (unsigned int i = 0; i < NumFramebuffers; i++) {
                /* Allocate a framebuffer. */
                m_framebuffers_mem[i] = m_pool_images->allocate(fb_size, fb_align);
                m_framebuffers[i].initialize(layout_framebuffer, m_framebuffers_mem[i].getMemBlock(), m_framebuffers_mem[i].getOffset());

                /* Generate a command list that binds it. */
                dk::ImageView color_target{ m_framebuffers[i] }, depth_target{ m_depth_buffer };
                m_cmd_buf.bindRenderTargets(&color_target, &depth_target);
                m_framebuffer_cmdlists[i] = m_cmd_buf.finishList();

                /* Fill in the array for use later by the swapchain creation code. */
                fb_array[i] = &m_framebuffers[i];
            }

            /* Create the swapchain using the framebuffers. */
            m_swapchain = dk::SwapchainMaker{m_device, nwindowGetDefault(), fb_array}.create();

            /* Generate the main rendering cmdlist. */
            this->RecordStaticCommands();
        }

        void DestroyFramebufferResources() {
            /* Return early if we have nothing to destroy. */
            if (!m_swapchain) return;

            /* Make sure the queue is idle before destroying anything. */
            m_queue.waitIdle();

            /* Clear the static cmdbuf, destroying the static cmdlists in the process. */
            m_cmd_buf.clear();

            /* Destroy the swapchain. */
            m_swapchain.destroy();

            /* Destroy the framebuffers. */
            for (unsigned int i = 0; i < NumFramebuffers; i ++) {
                m_framebuffers_mem[i].destroy();
            }

            /* Destroy the depth buffer. */
            m_depth_buffer_mem.destroy();
        }

        void RecordStaticCommands() {
            /* Initialize state structs with deko3d defaults. */
            dk::RasterizerState rasterizer_state;
            dk::ColorState color_state;
            dk::ColorWriteState color_write_state;

            /* Configure the viewport and scissor. */
            m_cmd_buf.setViewports(0, { { 0.0f, 0.0f, FramebufferWidth, FramebufferHeight, 0.0f, 1.0f } });
            m_cmd_buf.setScissors(0, { { 0, 0, FramebufferWidth, FramebufferHeight } });

            /* Clear the color and depth buffers. */
            m_cmd_buf.clearColor(0, DkColorMask_RGBA, 0.f, 0.f, 0.f, 1.0f);
            m_cmd_buf.clearDepthStencil(true, 1.0f, 0xFF, 0);

            /* Bind required state. */
            m_cmd_buf.bindRasterizerState(rasterizer_state);
            m_cmd_buf.bindColorState(color_state);
            m_cmd_buf.bindColorWriteState(color_write_state);

            m_render_cmdlist = m_cmd_buf.finishList();
        }

        void Render(u64 ns) {
            /* Acquire a framebuffer from the swapchain (and wait for it to be available). */
            int slot = m_queue.acquireImage(m_swapchain);

            /* Run the command list that attaches said framebuffer to the queue. */
            m_queue.submitCommands(m_framebuffer_cmdlists[slot]);

            /* Run the main rendering command list. */
            m_queue.submitCommands(m_render_cmdlist);

            nvgBeginFrame(m_vg, FramebufferWidth, FramebufferHeight, 1.0f);
            dbk::RenderMenu(m_vg, ns);
            nvgEndFrame(m_vg);

            /* Now that we are done rendering, present it to the screen. */
            m_queue.presentImage(m_swapchain, slot);
        }

    public:
        bool onFrame(u64 ns) override {
            dbk::UpdateMenu(ns);
            this->Render(ns);
            return !dbk::IsExitRequested();
        }
};

int main(int argc, char **argv) {
    /* Initialize the menu. */
    dbk::InitializeMenu(FramebufferWidth, FramebufferHeight);

    Daybreak daybreak;
    daybreak.run();
    return 0;
}
