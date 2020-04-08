from __future__ import annotations

from enum import IntEnum, auto, unique
from threading import Thread, Lock
from typing import Tuple, Callable, Any, Optional, Dict, Union, List
from inspect import iscoroutinefunction
from weakref import ref as weak_ref

import glfw
import curio
from pyrsistent import field

from pynotf.data import Value, RowHandle, Table, TableRow, Storage, set_value, RowHandleList
import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl


# DATA #################################################################################################################

@unique
class TableIndex(IntEnum):
    OPERATORS = 0
    FACTS = auto()


class OperatorColumns(TableRow):
    __table_index__: int = TableIndex.OPERATORS
    op_index = field(type=int, mandatory=True)
    schema_in = field(type=Value.Schema, mandatory=True)
    schema_out = field(type=Value.Schema, mandatory=True)
    args = field(type=Value, mandatory=True)
    data = field(type=Value, mandatory=True)
    downstream = field(type=RowHandle, mandatory=True)


class FactColumns(TableRow):
    __table_index__: int = TableIndex.FACTS
    schema = field(type=Value.Schema, mandatory=True)
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# APPLICATION ##########################################################################################################

class Application:

    def __init__(self):
        self._storage: Storage = Storage(
            operators=OperatorColumns,
            facts=FactColumns,
        )
        self._reactor: Reactor = Reactor(self)
        self._event_loop: EventLoop = EventLoop()

    def get_table(self, table_index: Union[int, TableIndex]) -> Table:
        return self._storage[int(table_index)]

    def schedule_event(self, callback: Callable, *args):
        self._event_loop.schedule((callback, *args))

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

        # ======================================================= HACK
        def key_callback_fn(_1, key, _2, action, _3):
            if action not in (glfw.PRESS, glfw.REPEAT):
                return

            if key == glfw.KEY_ESCAPE:
                glfw.set_window_should_close(window, 1)

            if glfw.KEY_A <= key <= glfw.KEY_Z:
                # if (mods & glfw.MOD_SHIFT) != glfw.MOD_SHIFT:
                #     key += 32
                # char: str = bytes([key]).decode()
                key_fact.next(Value())

        key_fact: Fact = self._reactor.create_fact("on_key", Value.Schema())
        click_counter = create_throttle(self._reactor, Value.Schema(), RowHandle(), 1.0)
        key_fact.subscribe(click_counter)
        glfw.set_key_callback(window, key_callback_fn)
        # =======================================================

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

    def __init__(self, application: Application):
        self._app: Application = application
        self._facts: Dict[str, weak_ref] = {}

    def create_operator(self, op_index: int, schema_in: Value.Schema, schema_out: Value.Schema, args: Value,
                        data: Value, downstream: RowHandle) -> RowHandle:
        assert op_index < len(OPERATOR_TABLE)
        operators_table: Table = self._app.get_table(TableIndex.OPERATORS)
        return operators_table.add_row(
            op_index=op_index, schema_in=schema_in, schema_out=schema_out, args=args, data=data, downstream=downstream)

    def create_fact(self, name: str, schema: Value.Schema) -> Fact:
        assert name not in self._facts
        facts_table: Table = self._app.get_table(TableIndex.FACTS)
        handle: RowHandle = facts_table.add_row(schema=schema)
        fact: Fact = Fact(self._app, handle)
        self._facts[name] = weak_ref(fact)
        return fact


class Operator:
    def __init__(self, application: Application, handle: RowHandle):
        self._app: Application = application
        self._handle: RowHandle = handle

    def is_valid(self) -> bool:
        return self._app.get_table(self._handle.table).is_handle_valid(self._handle)

    @property
    def data(self) -> Value:
        return self._app.get_table(self._handle.table)[self._handle]["data"]

    def __getitem__(self, name: str):
        return self._app.get_table(self._handle.table)[self._handle]["args"][name]

    def next(self, value: Value) -> None:
        self._emit(0, value)

    def fail(self, error: Value) -> None:
        self._emit(1, error)

    def complete(self) -> None:
        self._emit(2)

    def schedule(self, callback: Callable, *args):
        if iscoroutinefunction(callback):
            async def update_data_on_completion():
                self._app.get_table(self._handle.table)[self._handle]["data"] = await callback(*args)

            self._app.schedule_event(update_data_on_completion)
        else:
            self._app.schedule_event(lambda: callback(*args))

    def _emit(self, operator_row: int, value: Optional[Value] = None) -> None:
        assert 0 <= operator_row < 3

        table: Table = self._app.get_table(self._handle.table)
        downstream: RowHandle = table[self._handle]["downstream"]
        if not table.is_handle_valid(downstream):
            return  # operator has no downstream

        callback: Optional[Callable] = OPERATOR_TABLE[table[downstream]["op_index"]][operator_row]
        if callback is None:
            return

        if operator_row < 2:
            new_data: Value = callback(Operator(self._app, downstream), self._handle, value)
        else:
            new_data: Value = callback(Operator(self._app, downstream), self._handle)
        assert new_data.get_schema() == table[downstream]["data"].get_schema()
        table[downstream]['data'] = new_data


# FACT #################################################################################################################


class Fact:
    def __init__(self, application: Application, handle: RowHandle):
        self._app: Application = application
        self._handle: RowHandle = handle
        self._mutex: Lock = Lock()

    def get_schema(self) -> Value.Schema:
        facts_table: Table = self._app.get_table(TableIndex.FACTS)
        assert facts_table.is_handle_valid(self._handle)
        return facts_table[self._handle]['schema']

    def next(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        self._app.schedule_event((lambda: self._emit(0, value)))

    def fail(self, error: Value) -> None:
        self._app.schedule_event((lambda: self._emit(1, error)))

    def complete(self) -> None:
        self._app.schedule_event((lambda: self._emit(2)))

    def subscribe(self, operator: RowHandle):
        assert operator.table == TableIndex.OPERATORS
        assert self._app.get_table(operator.table)[operator]['schema_in'] == self.get_schema()
        facts_table: Table = self._app.get_table(TableIndex.FACTS)
        with self._mutex:
            downstream: RowHandleList = facts_table[self._handle]["downstream"]
            if operator not in downstream:
                facts_table[self._handle]["downstream"] = downstream.append(operator)

    def _emit(self, operator_row: int, value: Optional[Value] = None) -> None:
        assert 0 <= operator_row < 3

        operators_table: Table = self._app.get_table(TableIndex.OPERATORS)
        facts_table: Table = self._app.get_table(TableIndex.FACTS)

        # remove all expired downstream handles
        downstream: RowHandleList = facts_table[self._handle]["downstream"]
        expired: List[RowHandle] = []
        with self._mutex:
            for operator in downstream:
                if not operators_table.is_handle_valid(operator):
                    expired.append(operator)
            if expired:
                downstream = RowHandleList(op for op in downstream if op not in expired)
                facts_table[self._handle]["downstream"] = downstream

        # emit to all valid downstream operators
        for operator in downstream:

            # find the callback
            callback: Optional[Callable] = OPERATOR_TABLE[operators_table[operator]["op_index"]][operator_row]
            if callback is None:
                continue  # downstream operator type does not offer the requested callback

            # execute the callback
            if operator_row < 2:
                new_data: Value = callback(Operator(self._app, operator), self._handle, value)
            else:
                new_data: Value = callback(Operator(self._app, operator), self._handle)

            # update the operator data
            assert new_data.get_schema() == operators_table[operator]["data"].get_schema()
            operators_table[operator]['data'] = new_data


# THROTTLE #############################################################################################################

def create_throttle(reactor: Reactor, schema: Value.Schema, downstream: RowHandle, time_span: float) -> RowHandle:
    return reactor.create_operator(
        op_index=get_op_index(on_next=throttle_on_next),
        schema_in=schema,
        schema_out=schema.as_list(),
        args=Value(dict(time_span=time_span)),
        data=Value(dict(is_running=False, counter=0)),
        downstream=downstream)


def throttle_on_next(self: Operator, _1: RowHandle, _2: Value) -> Value:
    if not int(self.data["is_running"]) == 1:
        async def timeout():
            await curio.sleep(float(self["time_span"]))
            if not self.is_valid():
                return
            self.next(Value(self.data["counter"]))
            print(f'Clicked {int(self.data["counter"])} times in the last {float(self["time_span"])} seconds')
            return set_value(self.data, False, "is_running")

        self.schedule(timeout)
        return Value(dict(is_running=True, counter=1))

    else:
        return set_value(self.data, self.data["counter"] + 1, "counter")


# OPERATOR TABLE #######################################################################################################

OPERATOR_TABLE: Tuple[Tuple[Optional[Callable], Optional[Callable], Optional[Callable]], ...] = (
    (throttle_on_next, None, None),  # throttle
)


def get_op_index(on_next: Optional[Callable] = None,
                 on_failure: Optional[Callable] = None,
                 on_complete: Optional[Callable] = None) -> Optional[int]:
    """
    Returns the index of a callback in the Operator table.
    Used so I don't have to hard code indices into the create_* function calls.
    Checks all functions against the given row and asserts if not all match.
    :param on_next: The on_next function to look for.
    :param on_failure: The on_failure function to look for.
    :param on_complete: The on_complete function to look for.
    :return: The index of the given function/s.
    """
    assert on_next or on_failure or on_complete
    for index, (_next, _fail, _complete) in enumerate(OPERATOR_TABLE):
        if (on_next and on_next == _next
                or on_failure and on_failure == _fail
                or on_complete and on_complete == _complete):
            assert ((on_next is None or on_next == _next)
                    and (on_failure is None or on_failure == _fail)
                    and (on_complete is None or on_complete == _complete))
            return index
    return None


# class Map:  # TODO: in the example, this is a more generalized "map" operator with user-defined code
#     @staticmethod
#     def on_next(self: Operator, _, stream_id: int, value: Value) -> None:
#         self.emit(Value(len(value)))
#
#
# class Filter:  # TODO: again, this should probably take a user-defined expression
#     @staticmethod
#     def on_next(self: Operator, _1, _2, value: Value) -> None:
#         if int(value) >= 2:
#             self.emit(value)


# THROTTLE #############################################################################################################

if __name__ == "__main__":
    app: Application = Application()
    app.run()
