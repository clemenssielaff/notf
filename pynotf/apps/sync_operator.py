from __future__ import annotations

from threading import Thread
from typing import Tuple, Callable, Any, Optional
from inspect import iscoroutinefunction

import glfw
import curio
from pyrsistent import field

from pynotf.data import Value, RowHandle, Table, TableRow, Storage, set_value
import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl


# DATA #################################################################################################################

class OperatorColumns(TableRow):
    __table_index__: int = 0
    op_id = field(type=int, mandatory=True)
    args = field(type=Value, mandatory=True)
    data = field(type=Value, mandatory=True)
    downstream = field(type=RowHandle, mandatory=True)


# APPLICATION ##########################################################################################################

class Application:

    def __init__(self):
        self._storage: Storage = Storage(
            operators=OperatorColumns,
        )
        self._reactor: Reactor = Reactor(self, self._storage[OperatorColumns.__table_index__])
        self._event_loop: EventLoop = EventLoop()

    def schedule_event(self, callback: Tuple):
        self._event_loop.schedule(callback)

    def run(self):
        # initialize glfw
        if not glfw.init():
            return

        # create a windowed mode window and its OpenGL context.
        glfw.window_hint(glfw.CLIENT_API, glfw.OPENGL_ES_API)
        glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
        glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 2)
        glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
        glfw.window_hint(glfw.OPENGL_FORWARD_COMPAT, True)

        window = glfw.create_window(640, 480, "notf", None, None)
        if not window:
            glfw.terminate()
            return

        # make the window's context current
        glfw.make_context_current(window)
        nanovg._nanovg.loadGLES2Loader(glfw._glfw.glfwGetProcAddress)
        ctx = nanovg._nanovg.nvgCreateGLES3(5)

        # start the event loop
        event_thread = Thread(target=self._event_loop.run)
        event_thread.start()

        #======================================================= HACK
        def key_callback_fn(_1, key, _2, action, _3):
            if action not in (glfw.PRESS, glfw.REPEAT):
                return

            if key == glfw.KEY_ESCAPE:
                glfw.set_window_should_close(window, 1)

            if glfw.KEY_A <= key <= glfw.KEY_Z:
                # if (mods & glfw.MOD_SHIFT) != glfw.MOD_SHIFT:
                #     key += 32
                # char: str = bytes([key]).decode()
                new_data = clicker(Self(self._reactor, self._reactor._table, click_counter), RowHandle(), Value())
                assert new_data.get_schema() == self._reactor._table[click_counter]["data"].get_schema()
                print(int(new_data["counter"]))
                self._reactor._table[click_counter]["data"] = new_data

        click_counter = create_throttle(self._reactor, RowHandle(), 1.0)
        clicker: Callable = FUNCTION_TABLE[0][0]
        glfw.set_key_callback(window, key_callback_fn)
        #=======================================================

        try:
            while not glfw.window_should_close(window):
                width, height = glfw.get_window_size(window)
                frame_buffer_width, frame_buffer_height = glfw.get_framebuffer_size(window)
                gl.viewport(0, 0, frame_buffer_width, frame_buffer_height)
                gl.clearColor(0, 0, .2, 1)
                gl.clear(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT)

                gl.enable(gl.BLEND)
                gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
                gl.enable(gl.CULL_FACE)
                gl.disable(gl.DEPTH_TEST)

                nanovg.begin_frame(ctx, width, height, frame_buffer_width / width)

                nanovg.begin_path(ctx)
                nanovg.rounded_rect(ctx, 100, 100, max(0, width - 200), max(0, height - 200), 50)
                nanovg.fill_color(ctx, nanovg.rgba(.5, .5, .5))
                nanovg.fill(ctx)

                nanovg.end_frame(ctx)

                glfw.swap_buffers(window)
                glfw.wait_events()

        finally:
            self._event_loop.close()
            event_thread.join()

            nanovg._nanovg.nvgDeleteGLES3(ctx)
            glfw.terminate()


# EVENT LOOP ###########################################################################################################

class EventLoop:
    class _Close:
        pass

    def __init__(self):
        self._events = curio.UniversalQueue()

    def schedule(self, event) -> None:
        self._events.put(event)

    def close(self):
        self._events.put(self._Close())

    def run(self):
        curio.run(self._loop)

    async def _loop(self):
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
            event = await self._events.get()

            # close the loop
            if isinstance(event, self._Close):
                for task in active_tasks:
                    await task.join()
                return

            # execute a synchronous function first
            else:
                assert isinstance(event, tuple) and len(event) > 0
                if iscoroutinefunction(event[0]):
                    new_tasks.append(event)
                else:
                    event[0](*event[1:])

            # start all tasks that might have been kicked off by a (potential) synchronous function call above
            for task_args in new_tasks:
                active_tasks.add(await curio.spawn(finish_when_complete(*task_args)))
            new_tasks.clear()

            # finally join all finished task
            for task in finished_tasks:
                assert task in active_tasks
                await task.join()
                active_tasks.remove(task)
            finished_tasks.clear()


# REACTOR ##############################################################################################################

class Reactor:

    def __init__(self, application: Application, table: Table):
        self._application: Application = application
        self._table: Table = table

    def create_operator(self, op_id: int, args: Value, data: Value, downstream: RowHandle) -> RowHandle:
        return self._table.add_row(op_id=op_id, args=args, data=data, downstream=downstream)

    def emit_next(self, operator: RowHandle, value: Value) -> None:
        downstream: RowHandle = self._table[operator]["downstream"]
        if not self._table.is_handle_valid(downstream):
            return  # operator has no downstream

        callback: Optional[Callable] = FUNCTION_TABLE[self._table[downstream]["op_id"]][0]
        if callback is None:
            return

        new_data: Value = callback(Self(self, self._table, downstream), operator, value)
        assert new_data.get_schema() == self._table[operator]["data"].get_schema()
        self._table[operator]['data'] = new_data

    def emit_failure(self, operator: RowHandle, error: Value) -> None:
        downstream: RowHandle = self._table[operator]["downstream"]
        if not self._table.is_handle_valid(downstream):
            return  # operator has no downstream

        callback: Optional[Callable] = FUNCTION_TABLE[self._table[downstream]["op_id"]][1]
        if callback is None:
            return

        new_data: Value = callback(Self(self, self._table, downstream), operator, error)
        assert new_data.get_schema() == self._table[operator]["data"].get_schema()
        self._table[operator]['data'] = new_data

    def emit_completion(self, operator: RowHandle) -> None:
        downstream: RowHandle = self._table[operator]["downstream"]
        if not self._table.is_handle_valid(downstream):
            return  # operator has no downstream

        callback: Optional[Callable] = FUNCTION_TABLE[self._table[downstream]["op_id"]][2]
        if callback is None:
            return

        new_data: Value = callback(Self(self, self._table, downstream), operator)
        assert new_data.get_schema() == self._table[operator]["data"].get_schema()
        self._table[operator]['data'] = new_data

    def schedule(self, event: Tuple):
        self._application.schedule_event(event)


class Self:
    def __init__(self, reactor: Reactor, table: Table, handle: RowHandle):
        self._reactor: Reactor = reactor
        self._table: Table = table
        self._handle: RowHandle = handle

    def is_valid(self) -> bool:
        return self._table.is_handle_valid(self._handle)

    @property
    def data(self) -> Value:
        return self._table[self._handle]["data"]

    def __getitem__(self, name: str):
        return self._table[self._handle]["args"][name]

    def next(self, value: Value) -> None:
        self._reactor.emit_next(self._handle, value)

    def fail(self, value: Value) -> None:
        self._reactor.emit_failure(self._handle, value)

    def complete(self) -> None:
        self._reactor.emit_completion(self._handle)

    def schedule(self, callback: Callable, *args):
        self._reactor.schedule((callback, *args))


# THROTTLE #############################################################################################################

def create_throttle(reactor: Reactor, downstream: RowHandle, time_span: float) -> RowHandle:
    return reactor.create_operator(
        0, Value(dict(time_span=time_span)), Value(dict(is_running=False, counter=0)), downstream)


def throttle_on_next(self: Self, _1: RowHandle, _2: Value) -> Value:
    if not int(self.data["is_running"]) == 1:
        async def timeout():
            await curio.sleep(float(self["time_span"]))
            if not self.is_valid():
                return
            self.next(Value(self.data["counter"]))
            print(f'Clicked {int(self.data["counter"])} times in the last {float(self["time_span"])} seconds')

            # TODO: this is a hack
            self._table[self._handle]["data"] = set_value(self.data, False, "is_running")
            # return set_value(self.data, False, "is_running")

        self.schedule(timeout)
        return Value(dict(is_running=True, counter=1))

    else:
        return set_value(self.data, self.data["counter"] + 1, "counter")


FUNCTION_TABLE: Tuple[Tuple[Optional[Callable], Optional[Callable], Optional[Callable]], ...] = (
    (throttle_on_next, None, None),  # throttle
)

# class Map:  # TODO: in the example, this is a more generalized "map" operator with user-defined code
#     @staticmethod
#     def on_next(self: Self, _, stream_id: int, value: Value) -> None:
#         self.emit(Value(len(value)))
#
#
# class Filter:  # TODO: again, this should probably take a user-defined expression
#     @staticmethod
#     def on_next(self: Self, _1, _2, value: Value) -> None:
#         if int(value) >= 2:
#             self.emit(value)


# THROTTLE #############################################################################################################

if __name__ == "__main__":
    app: Application = Application()
    app.run()
