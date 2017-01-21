#pragma once

#include "painter_new.hpp"
#include "path.hpp"

namespace notf {

class RenderContext;

class OpenGLRenderBackend {
public: // methods

    int  renderCreate();
    int  renderCreateTexture(int type, int w, int h, int imageFlags, const unsigned char* data);
    int  renderDeleteTexture(int image);
    int  renderUpdateTexture(int image, int x, int y, int w, int h, const unsigned char* data);
    int  renderGetTextureSize(int image, int* w, int* h);
    void renderViewport(const RenderContext& context);
    void renderCancel();
    void renderFlush(const RenderContext& context);
    void renderFill(Painter::Paint* paint, Painter::Scissor* scissor, float fringe, const float* bounds, const Path* paths, int npaths);
    void renderStroke(Painter::Paint* paint, Painter::Scissor* scissor, float fringe, float strokeWidth, const Path* paths, int npaths);
    void renderTriangles(Painter::Paint* paint, Painter::Scissor* scissor, const Vertex* verts, int nverts);
    void renderDelete();
};

} // namespace notf
