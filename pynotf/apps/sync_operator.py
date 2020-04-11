from __future__ import annotations

import sys
from enum import IntEnum, auto, unique
from threading import Thread
from typing import Tuple, Callable, Any, Optional, Dict, Union, List, NamedTuple
from inspect import iscoroutinefunction

import glfw
import curio
from pyrsistent import field

from pynotf.data import Value, RowHandle, Table, TableRow, Storage, set_value, RowHandleList
import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl


# TODO: node hierarchy
# TODO: a service
# TODO: lua runtime
# TODO: graphic design

# UTILS ################################################################################################################

class IndexEnum(IntEnum):
    """
    Enum class whose auto indices start with zero.
    """

    def _generate_next_value_(self, _1, index: int, _2) -> int:
        return index


# DATA #################################################################################################################

@unique
class TableIndex(IntEnum):
    OPERATORS = 0
    SUBJECTS = auto()


class OperatorColumns(TableRow):
    __table_index__: int = TableIndex.OPERATORS
    op_index = field(type=int, mandatory=True)
    schema_in = field(type=Value.Schema, mandatory=True)
    schema_out = field(type=Value.Schema, mandatory=True)
    args = field(type=Value, mandatory=True)
    data = field(type=Value, mandatory=True)
    downstream = field(type=RowHandle, mandatory=True)


class SubjectColumns(TableRow):
    __table_index__: int = TableIndex.SUBJECTS
    schema = field(type=Value.Schema, mandatory=True)
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# FUNCTIONS ############################################################################################################

def run_downstream(app: Application, element: RowHandle, source: RowHandle, callback: OperatorCallback,
                   value: Value) -> None:
    """
    Execute one of the callbacks of a given downstream logic element.
    :param app: Application containing the state.
    :param element: Logic element whose callback to execute.
    :param source: Source Logic element that triggered the execution.
    :param callback: OperatorCallback to execute.
    :param value: OperatorCallback argument
    """
    if not app.is_handle_valid(element):
        return

    if element.table == TableIndex.OPERATORS:
        # find the callback to perform
        operator_table: Table = app.get_table(TableIndex.OPERATORS)
        callback_func: Optional[Callable] = OPERATOR_REGISTRY[operator_table[element]["op_index"]][callback]
        if callback_func is None:
            return  # operator type does not provide the requested callback

        # perform the callback ...
        new_data: Value = callback_func(Operator(app, element), source, value)

        # ...and update the operator's data
        assert new_data.get_schema() == operator_table[element]["data"].get_schema()
        operator_table[element]['data'] = new_data

    else:
        emit_from_subject(app, element, callback, value)


def emit_from_subject(app: Application, subject: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert app.is_handle_valid(subject)
    assert subject.table == TableIndex.SUBJECTS

    # temporarily clear the downstream of the emitting element to rule out cycles
    subject_table: Table = app.get_table(TableIndex.SUBJECTS)
    downstream: RowHandleList = subject_table[subject]["downstream"]
    subject_table[subject]["downstream"] = RowHandleList()

    if callback == OperatorCallback.NEXT:
        # emit to all valid downstream elements and remember the expired ones
        expired: List[RowHandle] = []
        for downstream_element in downstream:
            if app.is_handle_valid(downstream_element):
                run_downstream(app, downstream_element, subject, callback, value)
            else:
                expired.append(downstream_element)

        # remove all expired downstream handles
        if expired:
            downstream = RowHandleList(op for op in downstream if op not in expired)

        # restore the element's downstream after the emission has finished
        subject_table[subject]["downstream"] = downstream

    else:
        # emit one last time ...
        for downstream_element in downstream:
            run_downstream(app, downstream_element, subject, callback, value)

        # ... and invalidate the element
        subject_table.remove_row(subject)


def get_schema(app: Application, element: RowHandle) -> Value.Schema:
    assert app.is_handle_valid(element)
    if element.table == TableIndex.OPERATORS:
        return app.get_table(TableIndex.OPERATORS)[element]['schema_in']
    else:
        assert element.table == TableIndex.SUBJECTS
        return app.get_table(TableIndex.SUBJECTS)[element]['schema']


# APPLICATION ##########################################################################################################

class Application:

    def __init__(self):
        self._storage: Storage = Storage(
            operators=OperatorColumns,
            facts=SubjectColumns,
        )
        self._event_loop: EventLoop = EventLoop()
        self._scene: Scene = Scene(self)

    def get_table(self, table_index: Union[int, TableIndex]) -> Table:
        return self._storage[int(table_index)]

    def is_handle_valid(self, handle: RowHandle) -> bool:
        return self.get_table(handle.table).is_handle_valid(handle)

    def schedule_event(self, callback: Callable, *args):
        self._event_loop.schedule((callback, *args))

    def create_fact(self, schema: Value.Schema) -> Fact:
        return Fact(self, self.get_table(TableIndex.SUBJECTS).add_row(schema=schema))

    def get_subject(self, name: str) -> Optional[RowHandle]:
        return self._scene.get_subject(name)

    def run(self, setup_func: Callable[[Application, Any], None]) -> int:
        # initialize glfw
        if not glfw.init():
            return -1

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

        # make the window's context current
        glfw.make_context_current(window)
        nanovg._nanovg.loadGLES2Loader(glfw._glfw.glfwGetProcAddress)
        ctx = nanovg._nanovg.nvgCreateGLES3(5)

        # start the event loop
        event_thread = Thread(target=self._event_loop.run)
        event_thread.start()

        # execute the user-supplied setup function
        setup_func(self, window)

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

        return 0  # terminated normally


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


# LOGIC ################################################################################################################

class Operator:
    def __init__(self, application: Application, handle: RowHandle):
        assert (handle.table == TableIndex.OPERATORS)
        self._app: Application = application
        self._handle: RowHandle = handle

    @property
    def data(self) -> Value:
        return self._app.get_table(TableIndex.OPERATORS)[self._handle]["data"]

    def is_valid(self) -> bool:
        return self._app.get_table(TableIndex.OPERATORS).is_handle_valid(self._handle)

    def __getitem__(self, name: str):
        return self._app.get_table(TableIndex.OPERATORS)[self._handle]["args"][name]

    def next(self, value: Value) -> None:
        self._emit(OperatorCallback.NEXT, value)

    def fail(self, error: Value) -> None:
        self._emit(OperatorCallback.FAILURE, error)

    def complete(self) -> None:
        self._emit(OperatorCallback.COMPLETION, Value())

    def schedule(self, callback: Callable, *args):
        assert iscoroutinefunction(callback)

        async def update_data_on_completion():
            self._app.get_table(self._handle.table)[self._handle]["data"] = await callback(*args)

        self._app.schedule_event(update_data_on_completion)

    def _emit(self, callback: OperatorCallback, value: Value) -> None:
        operator_table: Table = self._app.get_table(TableIndex.OPERATORS)
        downstream: RowHandle = operator_table[self._handle]["downstream"]

        # if we emit failure or completion, invalidate the operator here to rule out cycles
        if callback != OperatorCallback.NEXT:
            operator_table.remove_row(self._handle)

        # execute the downstream callback
        if operator_table.is_handle_valid(downstream):
            run_downstream(self._app, downstream, self._handle, callback, value)


class Fact:
    def __init__(self, application: Application, handle: RowHandle):
        self._app: Application = application
        self._handle: RowHandle = handle

    def __del__(self):
        table: Table = self._app.get_table(TableIndex.SUBJECTS)
        if table.is_handle_valid(self._handle):
            table.remove_row(self._handle)

    def get_schema(self) -> Value.Schema:
        return self._app.get_table(TableIndex.SUBJECTS)[self._handle]['schema']

    def next(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        self._app.schedule_event(lambda: emit_from_subject(self._app, self._handle, OperatorCallback.NEXT, value))

    def fail(self, error: Value) -> None:
        self._app.schedule_event(lambda: emit_from_subject(self._app, self._handle, OperatorCallback.FAILURE, error))

    def complete(self) -> None:
        self._app.schedule_event(
            lambda: emit_from_subject(self._app, self._handle, OperatorCallback.COMPLETION, Value()))

    def subscribe(self, downstream: RowHandle):
        if not self._app.is_handle_valid(downstream):
            return
        assert self.get_schema() == get_schema(self._app, downstream)

        facts_table: Table = self._app.get_table(TableIndex.SUBJECTS)
        current_downstream: RowHandleList = facts_table[self._handle]["downstream"]
        if downstream not in current_downstream:
            facts_table[self._handle]["downstream"] = current_downstream.append(downstream)


# SCENE ################################################################################################################

class Scene:
    def __init__(self, app: Application):
        self._app: Application = app
        self._root_node: Node = Node(app, '', NodeDescription(
            inputs={}, outputs={}, state_machine=NodeStateMachine(states=dict(default=NodeNetworkDescription(
                operators=[],
                internal_connections=[],
                external_connections=[],
                incoming_connections=[],
                outgoing_connections=[],
            )), transitions=[], initial='default')))

    def create_node(self, name: str, description: NodeDescription):
        self._root_node._create_child(name, description)

    def get_subject(self, name: str) -> Optional[RowHandle]:
        path: List[str] = name.split('/')
        path[-1], subject_name = path[-1].split(':', maxsplit=1)
        node: Node = self._root_node
        for step in path:
            node = node.get_child(step)
        return node.get_subject(subject_name)


class NodeDescription(NamedTuple):
    inputs: Dict[str, Value.Schema]
    outputs: Dict[str, Value.Schema]
    state_machine: NodeStateMachine


class NodeStateMachine(NamedTuple):
    states: Dict[str, NodeNetworkDescription]
    transitions: List[Tuple[str, str]]
    initial: str


class NodeNetworkDescription(NamedTuple):
    operators: List[Tuple[OperatorKind, Value]]
    internal_connections: List[Tuple[int, int]]  # internal to internal
    external_connections: List[Tuple[str, str]]  # external to input
    incoming_connections: List[Tuple[str, int]]  # input to internal
    outgoing_connections: List[Tuple[int, str]]  # internal to output


class Node:
    def __init__(self, app: Application, name: str, description: NodeDescription, parent: Optional[Node] = None):
        self._app: Application = app
        self._name: str = name
        self._description: NodeDescription = description
        self._parent: Optional[Node] = parent
        self._children: Dict[str, Node] = {}
        self._network: List[RowHandle] = []
        self._state: str = ''

        assert len(set(description.inputs.keys()) | set(description.outputs.keys())) == \
               len(description.inputs) + len(description.outputs)
        subject_table: Table = self._app.get_table(TableIndex.SUBJECTS)
        self._inputs: Dict[str, RowHandle] = {
            name: subject_table.add_row(schema=schema) for (name, schema) in description.inputs.items()}
        self._outputs: Dict[str, RowHandle] = {
            name: subject_table.add_row(schema=schema) for (name, schema) in description.outputs.items()}

        self._transition_into(description.state_machine.initial)

    def __del__(self):
        self._clear_network()

        subject_table: Table = self._app.get_table(TableIndex.SUBJECTS)
        for direction in (self._inputs, self._outputs):
            for subject in direction.values():
                subject_table.remove_row(subject)

    def get_child(self, name: str) -> Optional[Node]:  # TODO: this should return a RowHandle - create a NodeTable
        return self._children.get(name)

    def get_subject(self, name: str) -> Optional[RowHandle]:
        if name in self._inputs:
            return self._inputs[name]
        if name in self._outputs:
            return self._outputs[name]
        return None

    def _transition_into(self, state: str) -> None:
        assert self._state == '' or (self._state, state) in self._description.state_machine.transitions

        self._clear_network()

        network_description: NodeNetworkDescription = self._description.state_machine.states[state]
        for kind, args in network_description.operators:
            self._network.append(OPERATOR_REGISTRY[kind][OperatorCallback.CREATE](self._app, args))

        subject_table: Table = self._app.get_table(TableIndex.SUBJECTS)
        for input_name, operator_index in network_description.incoming_connections:
            subject: RowHandle = self._inputs[input_name]
            current_downstream: RowHandleList = subject_table[subject]["downstream"]
            subject_table[subject]["downstream"] = current_downstream.append(self._network[operator_index])

        # TODO: create internal connections (and all others)

    def _clear_network(self) -> None:
        operator_table: Table = self._app.get_table(TableIndex.OPERATORS)
        for operator in self._network:
            operator_table.remove_row(operator)
        self._network.clear()

    # TODO: create child and -operator and state transition and all other private node methods should be operators...
    def _create_child(self, name: str, description: NodeDescription):
        assert name not in self._children
        child: Node = Node(self._app, name, description, self)
        self._children[name] = child


# OPERATOR REGISTRY ####################################################################################################

@unique
class OperatorCallback(IndexEnum):
    CREATE = auto()
    NEXT = auto()
    FAILURE = auto()
    COMPLETION = auto()


@unique
class OperatorKind(IndexEnum):
    BUFFER = auto()


def create_buffer_operator(app: Application, args: Value) -> RowHandle:
    schema: Value.Schema = Value.Schema.from_value(args['schema'])
    assert schema
    return app.get_table(TableIndex.OPERATORS).add_row(
        op_index=OperatorKind.BUFFER,
        schema_in=schema,
        schema_out=schema.as_list(),
        args=Value(dict(time_span=args['time_span'])),
        data=Value(dict(is_running=False, counter=0)),
        downstream=RowHandle()
    )


def buffer_on_next(self: Operator, _1: RowHandle, _2: Value) -> Value:
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


OPERATOR_REGISTRY: Tuple[
    Tuple[
        Callable[[Application, Value], RowHandle],
        Optional[Callable[[Operator, RowHandle], Value]],
        Optional[Callable[[Operator, RowHandle], Value]],
        Optional[Callable[[Operator, RowHandle], Value]],
    ], ...] = (
    (create_buffer_operator, buffer_on_next, None, None),  # buffer
)

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


# SETUP ################################################################################################################

count_presses_node: NodeDescription = NodeDescription(
    inputs=dict(key_down=Value.Schema()),
    outputs=dict(),
    state_machine=NodeStateMachine(
        states=dict(
            default=NodeNetworkDescription(
                operators=[(OperatorKind.BUFFER, Value(dict(schema=Value.Schema(), time_span=1)))],
                internal_connections=[],
                external_connections=[],
                incoming_connections=[('key_down', 0)],
                outgoing_connections=[],
            )
        ),
        transitions=[],
        initial='default'
    )
)


def app_setup(app: Application, window) -> None:
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

    key_fact: Fact = app.create_fact(Value.Schema())
    app._scene.create_node("herbert", count_presses_node)
    key_fact.subscribe(app.get_subject('herbert:key_down'))
    glfw.set_key_callback(window, key_callback_fn)

    # TODO: Add a state change via a `StateChange` Sink element, which exchanges the Nodes' network


# MAIN #################################################################################################################

def main() -> int:
    app: Application = Application()
    return app.run(app_setup)


if __name__ == "__main__":
    sys.exit(main())
