#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "nanovg.h"
#include "nanovg/dk_renderer.hpp"

#ifdef __cplusplus
extern "C" {
#endif

static int dknvg__maxi(int a, int b) { return a > b ? a : b; }

static const DKNVGtextureDescriptor* dknvg__findTexture(DKNVGcontext* dk, int id) {
    return dk->renderer->GetTextureDescriptor(*dk, id);
}

static int dknvg__renderCreate(void* uptr)
{
    DKNVGcontext *dk = (DKNVGcontext*)uptr;
    return dk->renderer->Create(*dk);
}

static int dknvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
    DKNVGcontext *dk = (DKNVGcontext*)uptr;
    return dk->renderer->CreateTexture(*dk, type, w, h, imageFlags, data);
}

static int dknvg__renderDeleteTexture(void* uptr, int image) {
    DKNVGcontext *dk = (DKNVGcontext*)uptr;
    return dk->renderer->DeleteTexture(*dk, image);
}

static int dknvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data) {
    DKNVGcontext *dk = (DKNVGcontext*)uptr;
    return dk->renderer->UpdateTexture(*dk, image, x, y, w, h, data);
}

static int dknvg__renderGetTextureSize(void* uptr, int image, int* w, int* h) {
    DKNVGcontext *dk = (DKNVGcontext*)uptr;
    return dk->renderer->GetTextureSize(*dk, image, w, h);
}

static void dknvg__xformToMat3x4(float* m3, float* t) {
    m3[0] = t[0];
    m3[1] = t[1];
    m3[2] = 0.0f;
    m3[3] = 0.0f;
    m3[4] = t[2];
    m3[5] = t[3];
    m3[6] = 0.0f;
    m3[7] = 0.0f;
    m3[8] = t[4];
    m3[9] = t[5];
    m3[10] = 1.0f;
    m3[11] = 0.0f;
}

static NVGcolor dknvg__premulColor(NVGcolor c) {
    c.r *= c.a;
    c.g *= c.a;
    c.b *= c.a;
    return c;
}

static int dknvg__convertPaint(DKNVGcontext* dk, DKNVGfragUniforms* frag, NVGpaint* paint,
                               NVGscissor* scissor, float width, float fringe, float strokeThr)
{
    const DKNVGtextureDescriptor *tex = NULL;
    float invxform[6];

    memset(frag, 0, sizeof(*frag));

    frag->innerCol = dknvg__premulColor(paint->innerColor);
    frag->outerCol = dknvg__premulColor(paint->outerColor);

    if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f) {
        memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
        frag->scissorExt[0] = 1.0f;
        frag->scissorExt[1] = 1.0f;
        frag->scissorScale[0] = 1.0f;
        frag->scissorScale[1] = 1.0f;
    } else {
        nvgTransformInverse(invxform, scissor->xform);
        dknvg__xformToMat3x4(frag->scissorMat, invxform);
        frag->scissorExt[0] = scissor->extent[0];
        frag->scissorExt[1] = scissor->extent[1];
        frag->scissorScale[0] = sqrtf(scissor->xform[0]*scissor->xform[0] + scissor->xform[2]*scissor->xform[2]) / fringe;
        frag->scissorScale[1] = sqrtf(scissor->xform[1]*scissor->xform[1] + scissor->xform[3]*scissor->xform[3]) / fringe;
    }

    memcpy(frag->extent, paint->extent, sizeof(frag->extent));
    frag->strokeMult = (width*0.5f + fringe*0.5f) / fringe;
    frag->strokeThr = strokeThr;

    if (paint->image != 0) {
        tex = dknvg__findTexture(dk, paint->image);
        if (tex == NULL) return 0;
        if ((tex->flags & NVG_IMAGE_FLIPY) != 0) {
            float m1[6], m2[6];
            nvgTransformTranslate(m1, 0.0f, frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, paint->xform);
            nvgTransformScale(m2, 1.0f, -1.0f);
            nvgTransformMultiply(m2, m1);
            nvgTransformTranslate(m1, 0.0f, -frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, m2);
            nvgTransformInverse(invxform, m1);
        } else {
            nvgTransformInverse(invxform, paint->xform);
        }
        frag->type = NSVG_SHADER_FILLIMG;

        if (tex->type == NVG_TEXTURE_RGBA)
            frag->texType = (tex->flags & NVG_IMAGE_PREMULTIPLIED) ? 0 : 1;
        else
            frag->texType = 2;
//		printf("frag->texType = %d\n", frag->texType);
    } else {
        frag->type = NSVG_SHADER_FILLGRAD;
        frag->radius = paint->radius;
        frag->feather = paint->feather;
        nvgTransformInverse(invxform, paint->xform);
    }

    dknvg__xformToMat3x4(frag->paintMat, invxform);

    return 1;
}

static DKNVGfragUniforms* nvg__fragUniformPtr(DKNVGcontext* dk, int i);

static void dknvg__renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
    NVG_NOTUSED(devicePixelRatio);
    DKNVGcontext* dk = (DKNVGcontext*)uptr;
    dk->view[0] = width;
    dk->view[1] = height;
}

static void dknvg__renderCancel(void* uptr) {
    DKNVGcontext* dk = (DKNVGcontext*)uptr;
    dk->nverts = 0;
    dk->npaths = 0;
    dk->ncalls = 0;
    dk->nuniforms = 0;
}

static int dknvg_convertBlendFuncFactor(int factor) {
    switch (factor) {
        case NVG_ZERO:
            return DkBlendFactor_Zero;
        case NVG_ONE:
            return DkBlendFactor_One;
        case NVG_SRC_COLOR:
            return DkBlendFactor_SrcColor;
        case NVG_ONE_MINUS_SRC_COLOR:
            return DkBlendFactor_InvSrcColor;
        case NVG_DST_COLOR:
            return DkBlendFactor_DstColor;
        case NVG_ONE_MINUS_DST_COLOR:
            return DkBlendFactor_InvDstColor;
        case NVG_SRC_ALPHA:
            return DkBlendFactor_SrcAlpha;
        case NVG_ONE_MINUS_SRC_ALPHA:
            return DkBlendFactor_InvSrcAlpha;
        case NVG_DST_ALPHA:
            return DkBlendFactor_DstAlpha;
        case NVG_ONE_MINUS_DST_ALPHA:
            return DkBlendFactor_InvDstAlpha;
        case NVG_SRC_ALPHA_SATURATE:
            return DkBlendFactor_SrcAlphaSaturate;
        default:
            return -1;
    }
}

static DKNVGblend dknvg__blendCompositeOperation(NVGcompositeOperationState op) {
    DKNVGblend blend;
    blend.srcRGB = dknvg_convertBlendFuncFactor(op.srcRGB);
    blend.dstRGB = dknvg_convertBlendFuncFactor(op.dstRGB);
    blend.srcAlpha = dknvg_convertBlendFuncFactor(op.srcAlpha);
    blend.dstAlpha = dknvg_convertBlendFuncFactor(op.dstAlpha);

    if (blend.srcRGB == -1 || blend.dstRGB == -1 || blend.srcAlpha == -1 || blend.dstAlpha == -1) {
        blend.srcRGB = DkBlendFactor_One;
        blend.dstRGB = DkBlendFactor_InvSrcAlpha;
        blend.srcAlpha = DkBlendFactor_One;
        blend.dstAlpha = DkBlendFactor_InvSrcAlpha;
    }
    return blend;
}

static void dknvg__renderFlush(void* uptr) {
    DKNVGcontext *dk = (DKNVGcontext*)uptr;
    dk->renderer->Flush(*dk);
}

static int dknvg__maxVertCount(const NVGpath* paths, int npaths) {
    int i, count = 0;
    for (i = 0; i < npaths; i++) {
        count += paths[i].nfill;
        count += paths[i].nstroke;
    }
    return count;
}

static DKNVGcall* dknvg__allocCall(DKNVGcontext* dk)
{
    DKNVGcall* ret = NULL;
    if (dk->ncalls+1 > dk->ccalls) {
        DKNVGcall* calls;
        int ccalls = dknvg__maxi(dk->ncalls+1, 128) + dk->ccalls/2; // 1.5x Overallocate
        calls = (DKNVGcall*)realloc(dk->calls, sizeof(DKNVGcall) * ccalls);
        if (calls == NULL) return NULL;
        dk->calls = calls;
        dk->ccalls = ccalls;
    }
    ret = &dk->calls[dk->ncalls++];
    memset(ret, 0, sizeof(DKNVGcall));
    return ret;
}

static int dknvg__allocPaths(DKNVGcontext* dk, int n)
{
    int ret = 0;
    if (dk->npaths+n > dk->cpaths) {
        DKNVGpath* paths;
        int cpaths = dknvg__maxi(dk->npaths + n, 128) + dk->cpaths/2; // 1.5x Overallocate
        paths = (DKNVGpath*)realloc(dk->paths, sizeof(DKNVGpath) * cpaths);
        if (paths == NULL) return -1;
        dk->paths = paths;
        dk->cpaths = cpaths;
    }
    ret = dk->npaths;
    dk->npaths += n;
    return ret;
}

static int dknvg__allocVerts(DKNVGcontext* dk, int n)
{
    int ret = 0;
    if (dk->nverts+n > dk->cverts) {
        NVGvertex* verts;
        int cverts = dknvg__maxi(dk->nverts + n, 4096) + dk->cverts/2; // 1.5x Overallocate
        verts = (NVGvertex*)realloc(dk->verts, sizeof(NVGvertex) * cverts);
        if (verts == NULL) return -1;
        dk->verts = verts;
        dk->cverts = cverts;
    }
    ret = dk->nverts;
    dk->nverts += n;
    return ret;
}

static int dknvg__allocFragUniforms(DKNVGcontext* dk, int n)
{
    int ret = 0, structSize = dk->fragSize;
    if (dk->nuniforms+n > dk->cuniforms) {
        unsigned char* uniforms;
        int cuniforms = dknvg__maxi(dk->nuniforms+n, 128) + dk->cuniforms/2; // 1.5x Overallocate
        uniforms = (unsigned char*)realloc(dk->uniforms, structSize * cuniforms);
        if (uniforms == NULL) return -1;
        dk->uniforms = uniforms;
        dk->cuniforms = cuniforms;
    }
    ret = dk->nuniforms * structSize;
    dk->nuniforms += n;
    return ret;
}

static DKNVGfragUniforms* nvg__fragUniformPtr(DKNVGcontext* dk, int i)
{
    return (DKNVGfragUniforms*)&dk->uniforms[i];
}

static void dknvg__vset(NVGvertex* vtx, float x, float y, float u, float v)
{
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
}

static void dknvg__renderFill(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe,
                              const float* bounds, const NVGpath* paths, int npaths)
{
    DKNVGcontext* dk = (DKNVGcontext*)uptr;
    DKNVGcall* call = dknvg__allocCall(dk);
    NVGvertex* quad;
    DKNVGfragUniforms* frag;
    int i, maxverts, offset;

    if (call == NULL) return;

    call->type = DKNVG_FILL;
    call->triangleCount = 4;
    call->pathOffset = dknvg__allocPaths(dk, npaths);
    if (call->pathOffset == -1) goto error;
    call->pathCount = npaths;
    call->image = paint->image;
    call->blendFunc = dknvg__blendCompositeOperation(compositeOperation);

    if (npaths == 1 && paths[0].convex)
    {
        call->type = DKNVG_CONVEXFILL;
        call->triangleCount = 0;	// Bounding box fill quad not needed for convex fill
    }

    // Allocate vertices for all the paths.
    maxverts = dknvg__maxVertCount(paths, npaths) + call->triangleCount;
    offset = dknvg__allocVerts(dk, maxverts);
    if (offset == -1) goto error;

    for (i = 0; i < npaths; i++) {
        DKNVGpath* copy = &dk->paths[call->pathOffset + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(DKNVGpath));
        if (path->nfill > 0) {
            copy->fillOffset = offset;
            copy->fillCount = path->nfill;
            memcpy(&dk->verts[offset], path->fill, sizeof(NVGvertex) * path->nfill);
            offset += path->nfill;
        }
        if (path->nstroke > 0) {
            copy->strokeOffset = offset;
            copy->strokeCount = path->nstroke;
            memcpy(&dk->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    // Setup uniforms for draw calls
    if (call->type == DKNVG_FILL) {
        // Quad
        call->triangleOffset = offset;
        quad = &dk->verts[call->triangleOffset];
        dknvg__vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
        dknvg__vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
        dknvg__vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
        dknvg__vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

        call->uniformOffset = dknvg__allocFragUniforms(dk, 2);
        if (call->uniformOffset == -1) goto error;
        // Simple shader for stencil
        frag = nvg__fragUniformPtr(dk, call->uniformOffset);
        memset(frag, 0, sizeof(*frag));
        frag->strokeThr = -1.0f;
        frag->type = NSVG_SHADER_SIMPLE;
        // Fill shader
        dknvg__convertPaint(dk, nvg__fragUniformPtr(dk, call->uniformOffset + dk->fragSize), paint, scissor, fringe, fringe, -1.0f);
    } else {
        call->uniformOffset = dknvg__allocFragUniforms(dk, 1);
        if (call->uniformOffset == -1) goto error;
        // Fill shader
        dknvg__convertPaint(dk, nvg__fragUniformPtr(dk, call->uniformOffset), paint, scissor, fringe, fringe, -1.0f);
    }

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (dk->ncalls > 0) dk->ncalls--;
}

static void dknvg__renderStroke(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor, float fringe,
                                float strokeWidth, const NVGpath* paths, int npaths)
{
    DKNVGcontext* dk = (DKNVGcontext*)uptr;
    DKNVGcall* call = dknvg__allocCall(dk);
    int i, maxverts, offset;

    if (call == NULL) {
        return;
    }

    call->type = DKNVG_STROKE;
    call->pathOffset = dknvg__allocPaths(dk, npaths);
    if (call->pathOffset == -1) goto error;
    call->pathCount = npaths;
    call->image = paint->image;
    call->blendFunc = dknvg__blendCompositeOperation(compositeOperation);

    // Allocate vertices for all the paths.
    maxverts = dknvg__maxVertCount(paths, npaths);
    offset = dknvg__allocVerts(dk, maxverts);
    if (offset == -1) goto error;

    for (i = 0; i < npaths; i++) {
        DKNVGpath* copy = &dk->paths[call->pathOffset + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(DKNVGpath));
        if (path->nstroke) {
            copy->strokeOffset = offset;
            copy->strokeCount = path->nstroke;
            memcpy(&dk->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    if (dk->flags & NVG_STENCIL_STROKES) {
        // Fill shader
        call->uniformOffset = dknvg__allocFragUniforms(dk, 2);
        if (call->uniformOffset == -1) goto error;

        dknvg__convertPaint(dk, nvg__fragUniformPtr(dk, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
        dknvg__convertPaint(dk, nvg__fragUniformPtr(dk, call->uniformOffset + dk->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f);
    } else {
        // Fill shader
        call->uniformOffset = dknvg__allocFragUniforms(dk, 1);
        if (call->uniformOffset == -1) goto error;

        dknvg__convertPaint(dk, nvg__fragUniformPtr(dk, call->uniformOffset), paint, scissor, strokeWidth, fringe, -1.0f);
    }

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (dk->ncalls > 0) dk->ncalls--;
}

static void dknvg__renderTriangles(void* uptr, NVGpaint* paint, NVGcompositeOperationState compositeOperation, NVGscissor* scissor,
                                   const NVGvertex* verts, int nverts, float fringe)
{
    DKNVGcontext* dk = (DKNVGcontext*)uptr;
    DKNVGcall* call = dknvg__allocCall(dk);
    DKNVGfragUniforms* frag;

    if (call == NULL) return;

    call->type = DKNVG_TRIANGLES;
    call->image = paint->image;
    call->blendFunc = dknvg__blendCompositeOperation(compositeOperation);

    // Allocate vertices for all the paths.
    call->triangleOffset = dknvg__allocVerts(dk, nverts);
    if (call->triangleOffset == -1) goto error;
    call->triangleCount = nverts;

    memcpy(&dk->verts[call->triangleOffset], verts, sizeof(NVGvertex) * nverts);

    // Fill shader
    call->uniformOffset = dknvg__allocFragUniforms(dk, 1);
    if (call->uniformOffset == -1) goto error;
    frag = nvg__fragUniformPtr(dk, call->uniformOffset);
    dknvg__convertPaint(dk, frag, paint, scissor, 1.0f, fringe, -1.0f);
    frag->type = NSVG_SHADER_IMG;

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (dk->ncalls > 0) dk->ncalls--;
}

static void dknvg__renderDelete(void* uptr) {
    DKNVGcontext* dk = (DKNVGcontext*)uptr;
    if (dk == NULL) return;

    free(dk->paths);
    free(dk->verts);
    free(dk->uniforms);
    free(dk->calls);

    free(dk);
}

NVGcontext* nvgCreateDk(nvg::DkRenderer *renderer, int flags) {
    NVGparams params;
    NVGcontext* ctx = NULL;
    DKNVGcontext* dk = (DKNVGcontext*)malloc(sizeof(DKNVGcontext));
    if (dk == NULL) goto error;
    memset(dk, 0, sizeof(DKNVGcontext));

    memset(&params, 0, sizeof(params));
    params.renderCreate = dknvg__renderCreate;
    params.renderCreateTexture = dknvg__renderCreateTexture;
    params.renderDeleteTexture = dknvg__renderDeleteTexture;
    params.renderUpdateTexture = dknvg__renderUpdateTexture;
    params.renderGetTextureSize = dknvg__renderGetTextureSize;
    params.renderViewport = dknvg__renderViewport;
    params.renderCancel = dknvg__renderCancel;
    params.renderFlush = dknvg__renderFlush;
    params.renderFill = dknvg__renderFill;
    params.renderStroke = dknvg__renderStroke;
    params.renderTriangles = dknvg__renderTriangles;
    params.renderDelete = dknvg__renderDelete;
    params.userPtr = dk;
    params.edgeAntiAlias = flags & NVG_ANTIALIAS ? 1 : 0;

    dk->renderer = renderer;
    dk->flags = flags;

    ctx = nvgCreateInternal(&params);
    if (ctx == NULL) goto error;

    return ctx;

error:
    // 'dk' is freed by nvgDeleteInternal.
    if (ctx != NULL) nvgDeleteInternal(ctx);
    return NULL;
}

void nvgDeleteDk(NVGcontext* ctx)
{
    nvgDeleteInternal(ctx);
}

#ifdef __cplusplus
}
#endif