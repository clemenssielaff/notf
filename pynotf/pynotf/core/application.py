from __future__ import annotations

from threading import Thread
from typing import Callable, Optional, Union

import glfw

import pynotf.extra.pynanovg as nanovg
from pycnotf import V2f, Size2f

from pynotf.data import Value, RowHandle, Table, Storage, Path
from pynotf.core.event_loop import EventLoop
from pynotf.core.utils import TableIndex
from pynotf.core.painterpreter import Painter
import pynotf.core as core


# APPLICATION ##########################################################################################################

class Application:

    def __init__(self):
        self._storage: Storage = Storage(
            operators=core.OperatorRow,
            nodes=core.NodeRow,
            layouts=core.LayoutRow,
        )
        self._event_loop: EventLoop = EventLoop()
        self._scene: core.Scene = core.Scene()

    def get_table(self, table_index: Union[int, TableIndex]) -> Table:
        return self._storage[int(table_index)]

    def is_handle_valid(self, handle: RowHandle) -> bool:
        if handle.is_null():
            return False
        return self.get_table(handle.table).is_handle_valid(handle)

    def get_scene(self) -> core.Scene:
        return self._scene

    @staticmethod
    def redraw():
        glfw.post_empty_event()

    def schedule_event(self, callback: Callable, *args):
        self._event_loop.schedule((callback, *args))

    def run(self, root_desc: core.NodeDescription) -> int:
        # initialize glfw
        if not glfw.init():
            return -1

        # create the application data
        self._scene.initialize(root_desc)

        # create a windowed mode window and its OpenGL context.
        glfw.window_hint(glfw.CLIENT_API, glfw.OPENGL_ES_API)
        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 2)
        glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, True)

        window = glfw.create_window(640, 480, "notf", None, None)
        if not window:
            glfw.terminate()
            return -1

        # TODO: some sort of ... OS service?
        #  also, where _do_ facts live? Right now, they live as source properties on the root node...
        glfw.set_key_callback(window, key_callback)
        glfw.set_mouse_button_callback(window, mouse_button_callback)
        glfw.set_window_size_callback(window, window_size_callback)

        # make the window's context current
        glfw.make_context_current(window)
        nanovg._nanovg.loadGLES2Loader(glfw._glfw.glfwGetProcAddress)
        ctx = nanovg._nanovg.nvgCreateGLES3(5)
        glfw.swap_interval(0)

        # start the event loop
        event_thread = Thread(target=self._event_loop.run)
        event_thread.start()

        # initialize the root
        self._scene.get_node(Path('/')).relayout_down(Size2f(640, 480))

        try:
            while not glfw.window_should_close(window):
                # TODO: this is happening in the MAIN loop - it should happen on a 3nd thread
                with Painter(window, ctx) as painter:
                    self._scene.paint(painter)
                glfw.wait_events()

        finally:
            self._event_loop.close()
            event_thread.join()
            self._scene.clear()

            nanovg._nanovg.nvgDeleteGLES3(ctx)
            glfw.terminate()

        return 0  # terminated normally


_THE_APPLICATION: Optional[Application] = None


def get_app() -> Application:
    global _THE_APPLICATION
    if _THE_APPLICATION is None:
        _THE_APPLICATION = Application()
    return _THE_APPLICATION


# SETUP ################################################################################################################

# noinspection PyUnusedLocal
def key_callback(window, key: int, scancode: int, action: int, mods: int) -> None:
    if action not in (glfw.PRESS, glfw.REPEAT):
        return

    if key == glfw.KEY_ESCAPE:
        glfw.set_window_should_close(window, 1)
        return

    if glfw.KEY_A <= key <= glfw.KEY_Z:
        get_app().get_scene().get_fact('key_fact').next(Value())


# noinspection PyUnusedLocal
def mouse_button_callback(window, button: int, action: int, mods: int) -> None:
    if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
        x, y = glfw.get_cursor_pos(window)
        get_app().get_scene().get_fact('mouse_fact').next(Value(x, y))
    elif button == glfw.MOUSE_BUTTON_RIGHT and action == glfw.PRESS:
        x, y = glfw.get_cursor_pos(window)
        for hitbox in get_app().get_scene().iter_hitboxes(V2f(x, y)):
            print(f"Triggered hitbox callback for hitbox with operator {hitbox.callback}")
        # get_app().get_scene().get_fact('mouse_fact').next(Value(x, y))


# noinspection PyUnusedLocal
def window_size_callback(window, width: int, height: int) -> None:
    get_app().get_scene().set_size(Size2f(width, height))
