from threading import Thread
from typing import Any

import glfw
import curio

import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl


async def arsch(msg):
    for _ in range(4):
        global color
        color = (1, 1, 1)
        glfw.post_empty_event()
        await curio.sleep(1)
    return msg

class Circuit:

    class _Close:
        pass

    def __init__(self):
        self._events = curio.UniversalQueue()

    def schedule(self, msg) -> None:
        self._events.put(msg)

    def close(self):
        self._events.put(self._Close())

    def run(self):
        curio.run(self._loop)

    def key_callback_fn(self, window, key, scancode, action, mods):
        if action not in (glfw.PRESS, glfw.REPEAT):
            return

        if key == glfw.KEY_ESCAPE:
            glfw.set_window_should_close(window, 1)

        if glfw.KEY_A <= key <= glfw.KEY_Z:
            if (mods & glfw.MOD_SHIFT) != glfw.MOD_SHIFT:
                key += 32
            char: str = bytes([key]).decode()
            self._events.put(char)

    async def _loop(self):
        global color

        new_tasks = []
        active_tasks = set()
        finished_tasks = []

        async def finish_when_complete(coroutine, *args) -> Any:
            """
            Executes a coroutine, and automatically joins
            :param coroutine: Coroutine to execute.
            :param args: Arguments passed to the coroutine.
            :return: Returns the result of the given coroutine (retrievable on join).
            """
            task_result: Any = await coroutine(*args)
            finished_tasks.append(await curio.current_task())
            return task_result

        while True:
            msg = await self._events.get()

            # close the loop
            if isinstance(msg, self._Close):
                for task in active_tasks:
                    await task.join()
                return

            # execute synchronous functions first
            elif msg == 'j':
                color = ((color[0] + 0.1) % 1.1, color[1], color[2])
            elif msg == 'k':
                color = (color[0], (color[1] + 0.1) % 1.1, color[2])
            elif msg == 'l':
                color = (color[0], color[1], (color[2] + 0.1) % 1.1)

            else:
                new_tasks.append((arsch, msg))

            # start all asynchronous coroutines
            for task_args in new_tasks:
                active_tasks.add(await curio.spawn(finish_when_complete(*task_args)))
            new_tasks.clear()

            # finally join all finished coroutines
            for task in finished_tasks:
                assert task in active_tasks
                result = await task.join()
                print(f"finished task with result {result}")
                active_tasks.remove(task)
            finished_tasks.clear()


def cursor_pos_fn(window, x: float, y: float):
    pass


color = (0., 0., 0.)


def main():
    # global window

    # Initialize the library.
    if not glfw.init():
        return

    # Create a windowed mode window and its OpenGL context.
    glfw.window_hint(glfw.CLIENT_API, glfw.OPENGL_ES_API)
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 2)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, True)

    window = glfw.create_window(640, 480, "Hello World", None, None)
    if not window:
        glfw.terminate()
        return

    # Make the window's context current.
    glfw.make_context_current(window)
    nanovg._nanovg.loadGLES2Loader(glfw._glfw.glfwGetProcAddress)
    ctx = nanovg._nanovg.nvgCreateGLES3(5)

    circuit = Circuit()
    glfw.set_key_callback(window, circuit.key_callback_fn)
    glfw.set_cursor_pos_callback(window, cursor_pos_fn)

    circuit_thread = Thread(target=circuit.run)
    circuit_thread.start()

    try:
        while not glfw.window_should_close(window):
            width, height = glfw.get_window_size(window)
            fbwidth, fbheight = glfw.get_framebuffer_size(window)
            gl.viewport(0, 0, fbwidth, fbheight)
            gl.clearColor(0, 0, .2, 1)
            gl.clear(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT)

            gl.enable(gl.BLEND)
            gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
            gl.enable(gl.CULL_FACE)
            gl.disable(gl.DEPTH_TEST)

            nanovg.begin_frame(ctx, width, height, fbwidth / width)

            nanovg.begin_path(ctx)
            nanovg.rounded_rect(ctx, 100, 100, max(0, width - 200), max(0, height - 200), 50)
            nanovg.fill_color(ctx, nanovg.rgba(*color))
            nanovg.fill(ctx)

            nanovg.end_frame(ctx)

            glfw.swap_buffers(window)
            glfw.wait_events()

    finally:
        circuit.close()
        circuit_thread.join()

        nanovg._nanovg.nvgDeleteGLES3(ctx)
        glfw.terminate()


if __name__ == "__main__":
    main()
