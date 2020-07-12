import sys
from ctypes import cast as c_cast, c_void_p

import glfw

from pycnotf import NanoVG, loadGLES2Loader
import pynotf.extra.opengl as gl


def main():
    if not glfw.init():
        return -1

    # create a windowed mode window and its OpenGL context.
    glfw.window_hint(glfw.CLIENT_API, glfw.OPENGL_ES_API)
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 2)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, True)

    window = glfw.create_window(800, 480, "notf", None, None)
    if not window:
        glfw.terminate()
        return -1

    # make the window's context current
    glfw.make_context_current(window)
    loadGLES2Loader(c_cast(glfw._glfw.glfwGetProcAddress, c_void_p).value)
    nvg: NanoVG = NanoVG()
    glfw.swap_interval(0)

    nvg.create_font("roboto", '/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf')

    try:
        while not glfw.window_should_close(window):
            window_width, window_height = glfw.get_window_size(window)
            fb_width, fb_height = glfw.get_framebuffer_size(window)
            gl.viewport(0, 0, fb_width, fb_height)
            gl.clearColor(0.098, 0.098, .098, 1)
            gl.clear(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT)

            gl.enable(gl.BLEND)
            gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
            gl.enable(gl.CULL_FACE)
            gl.disable(gl.DEPTH_TEST)

            nvg.begin_frame(window_width, window_height, fb_width / window_width)

            # draw_frame(nvg, 0, 0, window_width, window_height)

            nvg.end_frame()
            glfw.swap_buffers(window)
            glfw.wait_events()

    finally:
        del nvg
        glfw.terminate()

    return 0  # terminated normally


if __name__ == '__main__':
    sys.exit(main())
