#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/iosupport.h>

#include <switch.h>
#include <deko3d.h>

// Define the desired number of framebuffers
#define FB_NUM 2

// Define the size of the memory block that will hold code
#define CODEMEMSIZE (64*1024)

// Define the size of the memory block that will hold command lists
#define CMDMEMSIZE (64*1024)

#define NUM_IMAGE_SLOTS   1
#define NUM_SAMPLER_SLOTS 1

typedef struct {
    float pos[2];
    float tex[2];
} VertexDef;

typedef struct {
    float red;
    float green;
    float blue;
    float alpha;
} PaletteColor;

typedef struct {
    float dimensions[4];
    VertexDef vertices[3];
    PaletteColor palettes[24];
} ConsoleConfig;

static const VertexDef g_vertexData[3] = {
    { {  0.0f, +1.0f }, { 0.5f, 0.0f, } },
    { { -1.0f, -1.0f }, { 0.0f, 1.0f, } },
    { { +1.0f, -1.0f }, { 1.0f, 1.0f, } },
};

static const PaletteColor g_paletteData[24] = {
    { 0.0f,   0.0f,   0.0f,   0.0f }, // black
    { 0.5f,   0.0f,   0.0f,   1.0f }, // red
    { 0.0f,   0.5f,   0.0f,   1.0f }, // green
    { 0.5f,   0.5f,   0.0f,   1.0f }, // yellow
    { 0.0f,   0.0f,   0.5f,   1.0f }, // blue
    { 0.5f,   0.0f,   0.5f,   1.0f }, // magenta
    { 0.0f,   0.5f,   0.5f,   1.0f }, // cyan
    { 0.75f,  0.75f,  0.75f,  1.0f }, // white

    { 0.5f,   0.5f,   0.5f,   1.0f }, // bright black
    { 1.0f,   0.0f,   0.0f,   1.0f }, // bright red
    { 0.0f,   1.0f,   0.0f,   1.0f }, // bright green
    { 1.0f,   1.0f,   0.0f,   1.0f }, // bright yellow
    { 0.0f,   0.0f,   1.0f,   1.0f }, // bright blue
    { 1.0f,   0.0f,   1.0f,   1.0f }, // bright magenta
    { 0.0f,   1.0f,   1.0f,   1.0f }, // bright cyan
    { 1.0f,   1.0f,   1.0f,   1.0f }, // bright white

    { 0.0f,   0.0f,   0.0f,   0.0f }, // faint black
    { 0.25f,  0.0f,   0.0f,   1.0f }, // faint red
    { 0.0f,   0.25f,  0.0f,   1.0f }, // faint green
    { 0.25f,  0.25f,  0.0f,   1.0f }, // faint yellow
    { 0.0f,   0.0f,   0.25f,  1.0f }, // faint blue
    { 0.25f,  0.0f,   0.25f,  1.0f }, // faint magenta
    { 0.0f,   0.25f,  0.25f,  1.0f }, // faint cyan
    { 0.375f, 0.375f, 0.375f, 1.0f }, // faint white
};

typedef struct {
    uint16_t tileId;
    uint8_t frontPal;
    uint8_t backPal;
} ConsoleChar;

static const DkVtxAttribState g_attribState[] = {
    { .bufferId=0, .isFixed=0, .offset=offsetof(ConsoleChar,tileId),   .size=DkVtxAttribSize_1x16, .type=DkVtxAttribType_Uscaled, .isBgra=0 },
    { .bufferId=0, .isFixed=0, .offset=offsetof(ConsoleChar,frontPal), .size=DkVtxAttribSize_2x8,  .type=DkVtxAttribType_Uint,    .isBgra=0 },
};

static const DkVtxBufferState g_vtxbufState[] = {
    { .stride=sizeof(ConsoleChar), .divisor=1 },
};

struct GpuRenderer {
    ConsoleRenderer base;

    bool initialized;

    DkDevice device;
    DkQueue queue;

    DkMemBlock imageMemBlock;
    DkMemBlock codeMemBlock;
    DkMemBlock dataMemBlock;

    DkSwapchain swapchain;
    DkImage framebuffers[FB_NUM];
    DkImage tileset;
    ConsoleChar* charBuf;

    uint32_t codeMemOffset;
    DkShader vertexShader;
    DkShader fragmentShader;

    DkCmdBuf cmdbuf;
    DkCmdList cmdsBindFramebuffer[FB_NUM];
    DkCmdList cmdsRender;

    DkFence lastRenderFence;
};

static struct GpuRenderer* GpuRenderer(PrintConsole* con)
{
    return (struct GpuRenderer*)con->renderer;
}

static void GpuRenderer_destroy(struct GpuRenderer* r)
{
    // Make sure the queue is idle before destroying anything
    dkQueueWaitIdle(r->queue);

    // Destroy all the resources we've created
    dkQueueDestroy(r->queue);
    dkCmdBufDestroy(r->cmdbuf);
    dkSwapchainDestroy(r->swapchain);
    dkMemBlockDestroy(r->dataMemBlock);
    dkMemBlockDestroy(r->codeMemBlock);
    dkMemBlockDestroy(r->imageMemBlock);
    dkDeviceDestroy(r->device);

    // Clear out all state
    memset(&r->initialized, 0, sizeof(*r) - offsetof(struct GpuRenderer, initialized));
}

// Simple function for loading a shader from the filesystem
static void GpuRenderer_loadShader(struct GpuRenderer* r, DkShader* pShader, const char* path)
{
    // Open the file, and retrieve its size
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    rewind(f);

    // Look for a spot in the code memory block for loading this shader. Note that
    // we are just using a simple incremental offset; this isn't a general purpose
    // allocation algorithm.
    uint32_t codeOffset = r->codeMemOffset;
    r->codeMemOffset += (size + DK_SHADER_CODE_ALIGNMENT - 1) &~ (DK_SHADER_CODE_ALIGNMENT - 1);

    // Read the file into memory, and close the file
    fread((uint8_t*)dkMemBlockGetCpuAddr(r->codeMemBlock) + codeOffset, size, 1, f);
    fclose(f);

    // Initialize the user provided shader object with the code we've just loaded
    DkShaderMaker shaderMaker;
    dkShaderMakerDefaults(&shaderMaker, r->codeMemBlock, codeOffset);
    dkShaderInitialize(pShader, &shaderMaker);
}

static bool GpuRenderer_init(PrintConsole* con)
{
    struct GpuRenderer* r = GpuRenderer(con);

    if (r->initialized) {
        // We're already initialized
        return true;
    }

    // Create the deko3d device, which is the root object
    DkDeviceMaker deviceMaker;
    dkDeviceMakerDefaults(&deviceMaker);
    r->device = dkDeviceCreate(&deviceMaker);

    // Create the queue
    DkQueueMaker queueMaker;
    dkQueueMakerDefaults(&queueMaker, r->device);
    queueMaker.flags = DkQueueFlags_Graphics;
    r->queue = dkQueueCreate(&queueMaker);

    // Calculate required width/height for the framebuffers
    u32 width = con->font.tileWidth * con->consoleWidth;
    u32 height = con->font.tileHeight * con->consoleHeight;
    u32 totalConSize = con->consoleWidth * con->consoleHeight;

    // Calculate layout for the framebuffers
    DkImageLayoutMaker imageLayoutMaker;
    dkImageLayoutMakerDefaults(&imageLayoutMaker, r->device);
    imageLayoutMaker.flags = DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression;
    imageLayoutMaker.format = DkImageFormat_RGBA8_Unorm;
    imageLayoutMaker.dimensions[0] = width;
    imageLayoutMaker.dimensions[1] = height;

    // Calculate layout for the framebuffers
    DkImageLayout framebufferLayout;
    dkImageLayoutInitialize(&framebufferLayout, &imageLayoutMaker);

    // Calculate layout for the tileset
    dkImageLayoutMakerDefaults(&imageLayoutMaker, r->device);
    imageLayoutMaker.type = DkImageType_2DArray;
    imageLayoutMaker.format = DkImageFormat_R32_Float;
    imageLayoutMaker.dimensions[0] = con->font.tileWidth;
    imageLayoutMaker.dimensions[1] = con->font.tileHeight;
    imageLayoutMaker.dimensions[2] = con->font.numChars;

    // Calculate layout for the tileset
    DkImageLayout tilesetLayout;
    dkImageLayoutInitialize(&tilesetLayout, &imageLayoutMaker);

    // Retrieve necessary size and alignment for the framebuffers
    uint32_t framebufferSize  = dkImageLayoutGetSize(&framebufferLayout);
    uint32_t framebufferAlign = dkImageLayoutGetAlignment(&framebufferLayout);
    framebufferSize = (framebufferSize + framebufferAlign - 1) &~ (framebufferAlign - 1);

    // Retrieve necessary size and alignment for the tileset
    uint32_t tilesetSize  = dkImageLayoutGetSize(&tilesetLayout);
    uint32_t tilesetAlign = dkImageLayoutGetAlignment(&tilesetLayout);
    tilesetSize = (tilesetSize + tilesetAlign - 1) &~ (tilesetAlign - 1);

    // Create a memory block that will host the framebuffers and the tileset
    DkMemBlockMaker memBlockMaker;
    dkMemBlockMakerDefaults(&memBlockMaker, r->device, FB_NUM*framebufferSize + tilesetSize);
    memBlockMaker.flags = DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image;
    r->imageMemBlock = dkMemBlockCreate(&memBlockMaker);

    // Initialize the framebuffers with the layout and backing memory we've just created
    DkImage const* swapchainImages[FB_NUM];
    for (unsigned i = 0; i < FB_NUM; i ++) {
        swapchainImages[i] = &r->framebuffers[i];
        dkImageInitialize(&r->framebuffers[i], &framebufferLayout, r->imageMemBlock, i*framebufferSize);
    }

    // Create a swapchain out of the framebuffers we've just initialized
    DkSwapchainMaker swapchainMaker;
    dkSwapchainMakerDefaults(&swapchainMaker, r->device, nwindowGetDefault(), swapchainImages, FB_NUM);
    r->swapchain = dkSwapchainCreate(&swapchainMaker);

    // Initialize the tileset
    dkImageInitialize(&r->tileset, &tilesetLayout, r->imageMemBlock, FB_NUM*framebufferSize);

    // Create a memory block onto which we will load shader code
    dkMemBlockMakerDefaults(&memBlockMaker, r->device, CODEMEMSIZE);
    memBlockMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code;
    r->codeMemBlock = dkMemBlockCreate(&memBlockMaker);
    r->codeMemOffset = 0;

    // Load our shaders (both vertex and fragment)
    romfsInit();
    GpuRenderer_loadShader(r, &r->vertexShader, "romfs:/shaders/console_vsh.dksh");
    GpuRenderer_loadShader(r, &r->fragmentShader, "romfs:/shaders/console_fsh.dksh");

    // Generate the descriptors
    struct {
        DkImageDescriptor images[NUM_IMAGE_SLOTS];
        DkSamplerDescriptor samplers[NUM_SAMPLER_SLOTS];
    } descriptors;

    // Generate a image descriptor for the tileset
    DkImageView tilesetView;
    dkImageViewDefaults(&tilesetView, &r->tileset);
    dkImageDescriptorInitialize(&descriptors.images[0], &tilesetView, false, false);

    // Generate a sampler descriptor for the tileset
    DkSampler sampler;
    dkSamplerDefaults(&sampler);
    sampler.wrapMode[0] = DkWrapMode_ClampToEdge;
    sampler.wrapMode[1] = DkWrapMode_ClampToEdge;
    sampler.minFilter = DkFilter_Nearest;
    sampler.magFilter = DkFilter_Nearest;
    dkSamplerDescriptorInitialize(&descriptors.samplers[0], &sampler);

    uint32_t descriptorsOffset = CMDMEMSIZE;
    uint32_t configOffset = (descriptorsOffset + sizeof(descriptors) + DK_UNIFORM_BUF_ALIGNMENT - 1) &~ (DK_UNIFORM_BUF_ALIGNMENT - 1);
    uint32_t configSize = (sizeof(ConsoleConfig) + DK_UNIFORM_BUF_ALIGNMENT - 1) &~ (DK_UNIFORM_BUF_ALIGNMENT - 1);

    uint32_t charBufOffset = configOffset + configSize;
    uint32_t charBufSize   = totalConSize * sizeof(ConsoleChar);

    // Create a memory block which will be used for recording command lists using a command buffer
    dkMemBlockMakerDefaults(&memBlockMaker, r->device,
        (charBufOffset + charBufSize + DK_MEMBLOCK_ALIGNMENT - 1) &~ (DK_MEMBLOCK_ALIGNMENT - 1)
    );
    memBlockMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached;
    r->dataMemBlock = dkMemBlockCreate(&memBlockMaker);

    // Create a command buffer object
    DkCmdBufMaker cmdbufMaker;
    dkCmdBufMakerDefaults(&cmdbufMaker, r->device);
    r->cmdbuf = dkCmdBufCreate(&cmdbufMaker);

    // Feed our memory to the command buffer so that we can start recording commands
    dkCmdBufAddMemory(r->cmdbuf, r->dataMemBlock, 0, CMDMEMSIZE);

    // Create a temporary buffer that will hold the tileset
    dkMemBlockMakerDefaults(&memBlockMaker, r->device,
        (sizeof(float)*con->font.tileWidth*con->font.tileHeight*con->font.numChars + DK_MEMBLOCK_ALIGNMENT - 1) &~ (DK_MEMBLOCK_ALIGNMENT - 1)
    );
    memBlockMaker.flags = DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached;
    DkMemBlock scratchMemBlock = dkMemBlockCreate(&memBlockMaker);
    float* scratchMem = (float*)dkMemBlockGetCpuAddr(scratchMemBlock);

    // Unpack 1bpp tileset into a texture image the GPU can read
    unsigned packedTileWidth = (con->font.tileWidth+7)/8;
    for (unsigned tile = 0; tile < con->font.numChars; tile ++) {
        const uint8_t* data = (const uint8_t*)con->font.gfx + con->font.tileHeight*packedTileWidth*tile;
        for (unsigned y = 0; y < con->font.tileHeight; y ++) {
            const uint8_t* row = &data[packedTileWidth*(y+1)];
            uint8_t c = 0;
            for (unsigned x = 0; x < con->font.tileWidth; x ++) {
                if (!(x & 7))
                    c = *--row;
                *scratchMem++ = (c & 0x80) ? 1.0f : 0.0f;
                c <<= 1;
            }
        }
    }

    // Set up configuration
    DkGpuAddr configAddr = dkMemBlockGetGpuAddr(r->dataMemBlock) + configOffset;
    ConsoleConfig consoleConfig = {};
    consoleConfig.dimensions[0] = width;
    consoleConfig.dimensions[1] = height;
    consoleConfig.dimensions[2] = con->consoleWidth;
    consoleConfig.dimensions[3] = con->consoleHeight;
    memcpy(consoleConfig.vertices, g_vertexData, sizeof(g_vertexData));
    memcpy(consoleConfig.palettes, g_paletteData, sizeof(g_paletteData));

    // Generate a temporary command list for uploading stuff and run it
    DkGpuAddr descriptorSet = dkMemBlockGetGpuAddr(r->dataMemBlock) + descriptorsOffset;
    DkCopyBuf copySrc = { dkMemBlockGetGpuAddr(scratchMemBlock), 0, 0 };
    DkImageRect copyDst = { 0, 0, 0, con->font.tileWidth, con->font.tileHeight, con->font.numChars };
    dkCmdBufPushData(r->cmdbuf, descriptorSet, &descriptors, sizeof(descriptors));
    dkCmdBufPushConstants(r->cmdbuf, configAddr, configSize, 0, sizeof(consoleConfig), &consoleConfig);
    dkCmdBufBindImageDescriptorSet(r->cmdbuf, descriptorSet, NUM_IMAGE_SLOTS);
    dkCmdBufBindSamplerDescriptorSet(r->cmdbuf, descriptorSet + NUM_IMAGE_SLOTS*sizeof(DkImageDescriptor), NUM_SAMPLER_SLOTS);
    dkCmdBufCopyBufferToImage(r->cmdbuf, &copySrc, &tilesetView, &copyDst, 0);
    dkQueueSubmitCommands(r->queue, dkCmdBufFinishList(r->cmdbuf));
    dkQueueFlush(r->queue);
    dkQueueWaitIdle(r->queue);
    dkCmdBufClear(r->cmdbuf);

    // Destroy the scratch memory block since we don't need it anymore
    dkMemBlockDestroy(scratchMemBlock);

    // Retrieve the address of the character buffer
    DkGpuAddr charBufAddr = dkMemBlockGetGpuAddr(r->dataMemBlock) + charBufOffset;
    r->charBuf = (ConsoleChar*)((uint8_t*)dkMemBlockGetCpuAddr(r->dataMemBlock) + charBufOffset);
    memset(r->charBuf, 0, charBufSize);

    // Generate a command list for each framebuffer, which will bind each of them as a render target
    for (unsigned i = 0; i < FB_NUM; i ++) {
        DkImageView imageView;
        dkImageViewDefaults(&imageView, &r->framebuffers[i]);
        dkCmdBufBindRenderTarget(r->cmdbuf, &imageView, NULL);
        r->cmdsBindFramebuffer[i] = dkCmdBufFinishList(r->cmdbuf);
    }

    // Declare structs that will be used for binding state
    DkViewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    DkScissor scissor = { 0, 0, width, height };
    DkShader const* shaders[] = { &r->vertexShader, &r->fragmentShader };
    DkRasterizerState rasterizerState;
    DkColorState colorState;
    DkColorWriteState colorWriteState;

    // Initialize state structs with the deko3d defaults
    dkRasterizerStateDefaults(&rasterizerState);
    dkColorStateDefaults(&colorState);
    dkColorWriteStateDefaults(&colorWriteState);

    rasterizerState.fillRectangleEnable = true;
    colorState.alphaCompareOp = DkCompareOp_Greater;

    // Generate the main rendering command list
    dkCmdBufSetViewports(r->cmdbuf, 0, &viewport, 1);
    dkCmdBufSetScissors(r->cmdbuf, 0, &scissor, 1);
    //dkCmdBufClearColorFloat(r->cmdbuf, 0, DkColorMask_RGBA, 0.125f, 0.294f, 0.478f, 0.0f);
    dkCmdBufClearColorFloat(r->cmdbuf, 0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 0.0f);
    dkCmdBufBindShaders(r->cmdbuf, DkStageFlag_GraphicsMask, shaders, sizeof(shaders)/sizeof(shaders[0]));
    dkCmdBufBindRasterizerState(r->cmdbuf, &rasterizerState);
    dkCmdBufBindColorState(r->cmdbuf, &colorState);
    dkCmdBufBindColorWriteState(r->cmdbuf, &colorWriteState);
    dkCmdBufBindUniformBuffer(r->cmdbuf, DkStage_Vertex, 0, configAddr, configSize);
    dkCmdBufBindTexture(r->cmdbuf, DkStage_Fragment, 0, dkMakeTextureHandle(0, 0));
    dkCmdBufBindVtxAttribState(r->cmdbuf, g_attribState, sizeof(g_attribState)/sizeof(g_attribState[0]));
    dkCmdBufBindVtxBufferState(r->cmdbuf, g_vtxbufState, sizeof(g_vtxbufState)/sizeof(g_vtxbufState[0]));
    dkCmdBufBindVtxBuffer(r->cmdbuf, 0, charBufAddr, charBufSize);
    dkCmdBufSetAlphaRef(r->cmdbuf, 0.0f);
    dkCmdBufDraw(r->cmdbuf, DkPrimitive_Triangles, 3, totalConSize, 0, 0);
    r->cmdsRender = dkCmdBufFinishList(r->cmdbuf);

    r->initialized = true;
    return true;
}

static void GpuRenderer_deinit(PrintConsole* con)
{
    struct GpuRenderer* r = GpuRenderer(con);

    if (r->initialized) {
        GpuRenderer_destroy(r);
    }
}

static void GpuRenderer_drawChar(PrintConsole* con, int x, int y, int c)
{
    struct GpuRenderer* r = GpuRenderer(con);

    int writingColor = con->fg;
    int screenColor = con->bg;

    if (con->flags & CONSOLE_COLOR_BOLD) {
        writingColor += 8;
    } else if (con->flags & CONSOLE_COLOR_FAINT) {
        writingColor += 16;
    }

    if (con->flags & CONSOLE_COLOR_REVERSE) {
        int tmp = writingColor;
        writingColor = screenColor;
        screenColor = tmp;
    }

    // Wait for the fence
    dkFenceWait(&r->lastRenderFence, UINT64_MAX);

    ConsoleChar* pos = &r->charBuf[y*con->consoleWidth+x];
    pos->tileId = c;
    pos->frontPal = writingColor;
    pos->backPal = screenColor;
}

static void GpuRenderer_scrollWindow(PrintConsole* con)
{
    struct GpuRenderer* r = GpuRenderer(con);

    // Wait for the fence
    dkFenceWait(&r->lastRenderFence, UINT64_MAX);

    // Perform the scrolling
    for (int y = 0; y < con->windowHeight-1; y ++) {
        memcpy(
            &r->charBuf[(con->windowY+y+0)*con->consoleWidth + con->windowX],
            &r->charBuf[(con->windowY+y+1)*con->consoleWidth + con->windowX],
            sizeof(ConsoleChar)*con->windowWidth);
    }
}

static void GpuRenderer_flushAndSwap(PrintConsole* con)
{
    struct GpuRenderer* r = GpuRenderer(con);

    // Acquire a framebuffer from the swapchain (and wait for it to be available)
    int slot = dkQueueAcquireImage(r->queue, r->swapchain);

    // Run the command list that binds said framebuffer as a render target
    dkQueueSubmitCommands(r->queue, r->cmdsBindFramebuffer[slot]);

    // Run the main rendering command list
    dkQueueSubmitCommands(r->queue, r->cmdsRender);

    // Signal the fence
    dkQueueSignalFence(r->queue, &r->lastRenderFence, false);

    // Now that we are done rendering, present it to the screen
    dkQueuePresentImage(r->queue, r->swapchain, slot);
}

static struct GpuRenderer s_gpuRenderer =
{
    {
        GpuRenderer_init,
        GpuRenderer_deinit,
        GpuRenderer_drawChar,
        GpuRenderer_scrollWindow,
        GpuRenderer_flushAndSwap,
    }
};

ConsoleRenderer* getDefaultConsoleRenderer(void)
{
    return &s_gpuRenderer.base;
}
