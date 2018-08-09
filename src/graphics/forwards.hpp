#pragma once

#include "common/forwards.hpp"
#include "graphics/opengl.hpp"

NOTF_OPEN_NAMESPACE

NOTF_DEFINE_SHARED_POINTERS(class, Font);
NOTF_DEFINE_SHARED_POINTERS(class, FragmentShader);
NOTF_DEFINE_SHARED_POINTERS(class, FrameBuffer);
NOTF_DEFINE_SHARED_POINTERS(class, GeometryShader);
NOTF_DEFINE_SHARED_POINTERS(class, Pipeline);
NOTF_DEFINE_SHARED_POINTERS(class, RenderBuffer);
NOTF_DEFINE_SHARED_POINTERS(class, Shader);
NOTF_DEFINE_SHARED_POINTERS(class, TesselationShader);
NOTF_DEFINE_SHARED_POINTERS(class, Texture);
NOTF_DEFINE_SHARED_POINTERS(class, VertexShader);

NOTF_DEFINE_UNIQUE_POINTERS(class, GraphicsContext);
NOTF_DEFINE_UNIQUE_POINTERS(class, VertexArrayType);
NOTF_DEFINE_UNIQUE_POINTERS(class, IndexArrayType);
NOTF_DEFINE_UNIQUE_POINTERS(class, FontManager);

NOTF_DEFINE_SHARED_POINTERS(class, FragmentRenderer);
NOTF_DEFINE_SHARED_POINTERS(class, Plotter);

template<typename>
class PrefabType;

template<typename>
class PrefabFactory;

class TheGraphicsSystem;

NOTF_CLOSE_NAMESPACE

struct GLFWwindow;
