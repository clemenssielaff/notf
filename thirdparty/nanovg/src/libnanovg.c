#include "glad.h"

#include "nanovg.h"
#define NANOVG_GLES3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void loadGLES2Loader(GLADloadproc loadproc)
{
    gladLoadGLES2Loader(loadproc);
}

#ifdef __cplusplus
}
#endif
