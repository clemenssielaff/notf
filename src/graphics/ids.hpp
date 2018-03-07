#pragma once

#include "common/id.hpp"
#include "graphics/forwards.hpp"

NOTF_OPEN_NAMESPACE

using RenderBufferId = IdType<RenderBuffer, GLuint>;

using FrameBufferId = IdType<FrameBuffer, GLuint>;

using ShaderId = IdType<Shader, GLuint>;

using TextureId = IdType<Texture, GLuint>;

using PipelineId = IdType<Pipeline, GLuint>;

NOTF_CLOSE_NAMESPACE
