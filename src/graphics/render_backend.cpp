#include "graphics/render_backend.hpp"

namespace notf {

#ifdef NOTF_OPENGL3
const RenderBackend::Type RenderBackend::type = Type::OPENGL_3
#else // default
const RenderBackend::Type RenderBackend::type = Type::GLES_3;
#endif

} // namespace notf
