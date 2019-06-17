import os

os.environ["LD_LIBRARY_PATH"] = os.environ.get("LD_LIBRARY_PATH", "") + r""":/home/clemens/code/notf/pynotf/lib/"""

import glfw
from OpenGL import GLES3 as gl
import pynanovg as nanovg


def key_callback_fn(window, key, scancode, action, mods):
    if key == glfw.KEY_ESCAPE and action == glfw.PRESS:
        glfw.set_window_should_close(window, 1)


def main():
    # Initialize the library
    if not glfw.init():
        return
    # Create a windowed mode window and its OpenGL context
    glfw.window_hint(glfw.CLIENT_API, glfw.OPENGL_ES_API)
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 2)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, True)
    window = glfw.create_window(640, 480, "Hello World", None, None)
    if not window:
        glfw.terminate()
        return

    glfw.set_key_callback(window, key_callback_fn)

    # Make the window's context current
    glfw.make_context_current(window)
    nanovg._nanovg.loadGLES2Loader(glfw._glfw.glfwGetProcAddress)
    ctx = nanovg._nanovg.nvgCreateGLES3(5)

    try:
        while not glfw.window_should_close(window):
            width, height = glfw.get_window_size(window)
            fbwidth, fbheight = glfw.get_framebuffer_size(window)
            gl.glViewport(0, 0, fbwidth, fbheight)
            gl.glClearColor(0, 0, .2, 1)
            gl.glClear(gl.GL_COLOR_BUFFER_BIT, gl.GL_DEPTH_BUFFER_BIT, gl.GL_STENCIL_BUFFER_BIT)

            gl.glEnable(gl.GL_BLEND)
            gl.glBlendFunc(gl.GL_SRC_ALPHA, gl.GL_ONE_MINUS_SRC_ALPHA)
            gl.glEnable(gl.GL_CULL_FACE)
            gl.glDisable(gl.GL_DEPTH_TEST)

            nanovg.begin_frame(ctx, width, height, fbwidth / width)

            nanovg.begin_path(ctx)
            nanovg.rounded_rect(ctx, 100, 100, max(0, width - 200), max(0, height - 200), 50)
            nanovg.fill_color(ctx, nanovg.rgba(1., 1., 1.))
            nanovg.fill(ctx)

            nanovg.translate(ctx, 1, 2)
            xform = nanovg.current_transform(ctx)
            # inverse, success = nanovg.transform_inverse(xform)
            # print("({})".format(", ".join(str(x) for x in xform)))
            # print("({})".format(", ".join(str(x) for x in inverse)))
            # print(success)
            # print("\n")
            # print(nanovg.transform_point(xform, 0, 0))
            # nanovg.transform_identity(xform)
            # print(nanovg.transform_point(xform, 0, 0))

            nanovg.end_frame(ctx)

            glfw.swap_buffers(window)
            glfw.wait_events()
    finally:
        nanovg._nanovg.nvgDeleteGLES3(ctx)
        glfw.terminate()


if __name__ == "__main__":
    main()
