#include "graphics2/backend.hpp"

namespace notf {

#ifdef NOTF_OPENGL2
const RenderBackend::Type RenderBackend::type = Type::OPENGL_2
#elif defined NOTF_OPENGL3
const RenderBackend::Type RenderBackend::type = Type::OPENGL_3
#elif defined NOTF_GLES2
const RenderBackend::Type RenderBackend::type = Type::GLES_2
#elif defined NOTF_GLES3
const RenderBackend::Type RenderBackend::type = Type::GLES_3
#else // default
const RenderBackend::Type RenderBackend::type = Type::GLES_3;
#endif

} // namespace notf
