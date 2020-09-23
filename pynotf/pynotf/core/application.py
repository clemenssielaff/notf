from __future__ import annotations

from ctypes import cast as c_cast, c_void_p
from threading import Thread
from typing import Callable, Optional, Union, Type, Dict, Tuple

import glfw

from pycnotf import V2f, Size2f, NanoVG, loadGLES2Loader
from pynotf.data import Value, RowHandle, Table, Storage, Path
import pynotf.core as core


# APPLICATION ##########################################################################################################

class Application:

    def __init__(self):
        self._storage: Storage = Storage(
            operators=core.OperatorRow,
            nodes=core.NodeRow,
            layouts=core.LayoutRow,
        )
        self._event_loop: core.EventLoop = core.EventLoop()
        self._scene: core.Scene = core.Scene()
        self._services: Dict[str, core.Service] = {}

    def register_service(self, name: str, service: Type) -> None:
        if name in self._services:
            raise NameError(f'Cannot register Service with duplicate name "{name}"')
        self._services[name] = service()

    def get_table(self, table_index: Union[int, core.TableIndex]) -> Table:
        return self._storage[int(table_index)]

    def is_handle_valid(self, handle: RowHandle) -> bool:
        if handle.is_null():
            return False
        return self.get_table(handle.table).is_handle_valid(handle)

    def get_scene(self) -> core.Scene:  # TODO: see if we can get rid of Application.get_scene
        return self._scene

    def get_node(self, path: Union[Path, str]) -> Optional[core.Node]:
        return self._scene.get_node(path)

    def get_operator(self, path: Union[Path, str]) -> Optional[core.Operator]:
        interop_name: Optional[str] = path.get_interop_name()
        if interop_name is not None:
            node: Optional[core.Node] = self.get_node(path)
            if node is None:
                return None
            return node.get_interop(interop_name)

        # service
        query: Optional[Tuple[str, str]] = path.get_service_query()
        if query is not None:
            service: Optional[core.Service] = self._services.get(query[0])
            if service is None:
                return None
            return service.get_fact(query[1])

    @staticmethod
    def redraw():
        glfw.post_empty_event()

    def schedule_event(self, callback: Callable, *args):
        self._event_loop.schedule((callback, *args))

    def run(self, root_description: Value) -> int:
        # initialize glfw
        if not glfw.init():
            return -1

        # start all services
        for service in self._services.values():
            service.initialize()

        # create the application data
        self._scene.initialize(root_description)

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

        # TODO: some sort of ... OS service?
        #  also, where _do_ facts live? Right now, they live as interface operators on the root node...
        glfw.set_key_callback(window, key_callback)
        glfw.set_mouse_button_callback(window, mouse_button_callback)
        glfw.set_window_size_callback(window, window_size_callback)

        # make the window's context current
        glfw.make_context_current(window)
        loadGLES2Loader(c_cast(glfw._glfw.glfwGetProcAddress, c_void_p).value)
        nanovg: NanoVG = NanoVG()
        glfw.swap_interval(0)

        # start the event loop
        event_thread = Thread(target=self._event_loop.run)
        event_thread.start()

        # initialize the root
        self._scene.get_node('/').relayout_down(Size2f(800, 480))  # TODO: make this dependent on the Window size

        # TODO: font management ... somehow (in-app? use thirdparty library?)
        nanovg.create_font("Roboto", "/home/clemens/code/notf/res/fonts/Roboto-Regular.ttf")

        try:
            while not glfw.window_should_close(window):
                # TODO: this is happening in the MAIN loop - it should happen on a 3rd thread
                with core.Painter(window, nanovg) as painter:
                    self._scene.paint(painter)

                self._scene.perform_node_transitions()
                glfw.wait_events()
                # glfw.poll_events()  # run as fast as you can

        finally:
            self._event_loop.close()
            event_thread.join()
            self._scene.clear()

            # shutdown all services
            for service in self._services.values():
                service.shutdown()

            del nanovg
            glfw.terminate()

        return 0  # terminated normally


_THE_APPLICATION: Optional[Application] = None


def get_app() -> Application:
    global _THE_APPLICATION
    if _THE_APPLICATION is None:
        _THE_APPLICATION = Application()
    return _THE_APPLICATION


# SETUP ################################################################################################################

# TODO: these callbacks rely on the existence of facts that need to be defined on the root node.
#   Maybe have another set of builtin interface ops for the root only? I think there's another to-do for this as well.

# noinspection PyUnusedLocal
def key_callback(window, key: int, scancode: int, action: int, mods: int) -> None:
    if action not in (glfw.PRESS, glfw.REPEAT):
        return

    if key == glfw.KEY_ESCAPE:
        glfw.set_window_should_close(window, 1)
        return

    if glfw.KEY_A <= key <= glfw.KEY_Z:
        get_app().get_scene().get_fact('key_fact').update(Value())


# noinspection PyUnusedLocal
def mouse_button_callback(window, button: int, action: int, mods: int) -> None:
    if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
        x, y = glfw.get_cursor_pos(window)
        get_app().get_scene().get_fact('mouse_fact').update(Value(x=x, y=y))
    elif button == glfw.MOUSE_BUTTON_RIGHT and action == glfw.PRESS:
        x, y = glfw.get_cursor_pos(window)
        hitbox: core.Sketch.Hitbox
        for hitbox in get_app().get_scene().iter_hitboxes(V2f(x, y)):
            if hitbox.callback.is_valid():
                hitbox.callback.update(Value(x=x, y=y))


# noinspection PyUnusedLocal
def window_size_callback(window, width: int, height: int) -> None:
    get_app().get_scene().set_size(Size2f(width, height))
