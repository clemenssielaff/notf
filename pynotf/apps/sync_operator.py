from __future__ import annotations

import sys
from enum import IntEnum, auto, unique, Enum
from threading import Thread
from typing import Tuple, Callable, Any, Optional, Dict, Union, List, NamedTuple
from inspect import iscoroutinefunction

import glfw
import curio
from pyrsistent import field

from pynotf.data import Value, RowHandle, Table, TableRow, Storage, set_value, RowHandleList, Bonsai, RowHandleMap
import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl


# * node hierarchy
# * a service (periphery devices)
# * graphic design
# * lua runtime

# UTILS ################################################################################################################

class IndexEnum(IntEnum):
    """
    Enum class whose auto indices start with zero.
    """

    def _generate_next_value_(self, _1, index: int, _2) -> int:
        return index


# DATA #################################################################################################################


@unique
class OperatorKind(IndexEnum):
    RELAY = auto()
    BUFFER = auto()


@unique
class TableIndex(IndexEnum):
    OPERATORS = auto()
    RELAYS = auto()
    NODES = auto()


@unique
class OperatorCallback(IntEnum):
    NEXT = 0  # 0-2 matches the corresponding EmitterStatus
    FAILURE = 1
    COMPLETION = 2
    CREATE = 3


@unique
class EmitterStatus(IntEnum):
    """
    Emitters start out IDLE and change to EMITTING whenever they are emitting a ValueSignal to connected Receivers.
    If an Emitter tries to emit but is already in EMITTING state, we know that we have caught a cyclic dependency.
    To emit a FailureSignal or a CompletionSignal, the Emitter will switch to the respective FAILING or COMPLETING
    state. Once it has finished its `_fail` or `_complete` methods, it will permanently change its state to
    COMPLETED or FAILED and will not emit anything again.

        --> IDLE <-> EMITTING
              |
              +--> FAILING --> FAILED
              |
              +--> COMPLETING --> COMPLETE
    """
    EMITTING = 0  # 0-2 matches the corresponding OperatorCallback
    FAILING = 1
    COMPLETING = 2
    IDLE = 3  # follow-up status is active (EMITTING, FAILING, COMPLETING) + 3
    FAILED = 4
    COMPLETED = 5

    def is_active(self) -> bool:
        """
        There are 3 active and 3 passive states:
            * IDLE, FAILED and COMPLETED are passive
            * EMITTING, FAILING and COMPLETING are active
        """
        return self < EmitterStatus.IDLE

    def is_completed(self) -> bool:
        """
        Every status except IDLE and EMITTING can be considered "completed".
        """
        return not (self == EmitterStatus.IDLE or self == EmitterStatus.EMITTING)

    def next_after(self) -> EmitterStatus:
        return EmitterStatus(self + 3)


@unique
class NodeSocketDirection(Enum):
    INPUT = auto()
    OUTPUT = auto()


class NodeNetworkDescription(NamedTuple):  # TODO: assign integers to sockets as well
    operators: List[Tuple[OperatorKind, Value]]
    internal_connections: List[Tuple[int, int]]  # internal to internal
    external_connections: List[Tuple[str, str]]  # external to input
    incoming_connections: List[Tuple[str, int]]  # input to internal
    outgoing_connections: List[Tuple[int, str]]  # internal to output


class NodeStateMachine(NamedTuple):
    states: Dict[str, NodeNetworkDescription]
    transitions: List[Tuple[str, str]]
    initial: str


class NodeSockets(NamedTuple):
    names: Bonsai
    elements: List[Tuple[NodeSocketDirection, RowHandle]]


class NodeDescription(NamedTuple):
    sockets: Dict[str, Tuple[NodeSocketDirection, Value.Schema]]
    state_machine: NodeStateMachine


class OperatorRow(TableRow):
    __table_index__: int = TableIndex.OPERATORS
    value = field(type=Value, mandatory=True)
    op_index = field(type=int, mandatory=True)
    schema = field(type=Value.Schema, mandatory=True)  # input schema
    args = field(type=Value, mandatory=True)
    data = field(type=Value, mandatory=True)
    status = field(type=EmitterStatus, mandatory=True, initial=EmitterStatus.IDLE)
    downstream = field(type=RowHandle, mandatory=True, initial=RowHandle())


class RelayRow(TableRow):
    __table_index__: int = TableIndex.RELAYS
    value = field(type=Value, mandatory=True)
    status = field(type=EmitterStatus, mandatory=True, initial=EmitterStatus.IDLE)
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


class NodeRow(TableRow):
    __table_index__: int = TableIndex.NODES
    description = field(type=NodeDescription, mandatory=True)
    parent = field(type=RowHandle, mandatory=True)
    sockets = field(type=NodeSockets, mandatory=True)
    state = field(type=str, mandatory=True, initial='')
    children = field(type=RowHandleMap, mandatory=True, initial=RowHandleMap())
    network = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# FUNCTIONS ############################################################################################################

def run_downstream(element: RowHandle, source: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert get_app().is_handle_valid(element)
    assert get_app().is_handle_valid(source)

    if element.table == TableIndex.OPERATORS:
        run_operator(element, source, callback, value)
    else:
        assert element.table == TableIndex.RELAYS
        emit_from_relay(element, callback, value)


def run_operator(operator: RowHandle, source: RowHandle, callback: OperatorCallback, value: Value) -> None:
    # make sure the operator is valid and not completed yet
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    status: EmitterStatus = operator_table[operator]['status']
    if status.is_completed():
        return  # operator has completed

    # find the callback to perform
    callback_func: Optional[Callable] = ELEMENT_TABLE[operator_table[operator]["op_index"]][callback]
    if callback_func is None:
        return  # operator type does not provide the requested callback

    # perform the callback ...
    new_data: Value = callback_func(Operator(operator), source, value)

    # ...and update the operator's data
    assert new_data.get_schema() == operator_table[operator]["data"].get_schema()
    operator_table[operator]['data'] = new_data


def emit_from_relay(relay: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert callback != OperatorCallback.CREATE

    # make sure the relay is valid and not completed yet
    relay_table: Table = get_app().get_table(TableIndex.RELAYS)
    status: EmitterStatus = relay_table[relay]['status']
    if status.is_completed():
        return  # relay has completed

    # ensure that the relay is able to emit the given value
    assert value.get_schema() == relay_table[relay]['value'].get_schema()

    # mark the relay as actively emitting
    assert not status.is_active()  # cyclic error
    relay_table[relay]['status'] = EmitterStatus(callback)  # set the active status

    # store the emitted value
    relay_table[relay]['value'] = value

    if callback == OperatorCallback.NEXT:
        # emit to all valid downstream elements and remember the expired ones
        expired: List[RowHandle] = []
        for downstream_element in relay_table[relay]["downstream"]:
            if get_app().is_handle_valid(downstream_element):
                run_downstream(downstream_element, relay, callback, value)
            else:
                expired.append(downstream_element)

        # remove all expired downstream handles
        if expired:
            relay_table[relay]["downstream"] = RowHandleList(op for op in relay_table[relay]["downstream"]
                                                             if op not in expired)

    else:
        # emit one last time ...
        for downstream_element in relay_table[relay]["downstream"]:
            run_downstream(downstream_element, relay, callback, value)

        # ... and remove all downstream handles
        relay_table[relay]["downstream"] = RowHandleList()

    # reset the status
    relay_table[relay]['status'] = EmitterStatus(callback).next_after()


def emit_from_operator(operator: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert callback != OperatorCallback.CREATE

    # make sure the operator is valid and not completed yet
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    status: EmitterStatus = operator_table[operator]['status']
    if status.is_completed():
        return  # operator has completed

    # ensure that the operator is able to emit the given value
    assert value.get_schema() == operator_table[operator]['value'].get_schema()

    # mark the operator as actively emitting
    assert not status.is_active()  # cyclic error
    operator_table[operator]['status'] = EmitterStatus(callback)  # set the active status

    # store the emitted value
    operator_table[operator]['value'] = value

    # emit or remove the downstream if it has expired
    downstream: RowHandle = operator_table[operator]["downstream"]
    if downstream.is_null():
        if get_app().is_handle_valid(downstream):
            run_downstream(downstream, operator, callback, value)
        else:
            operator_table[operator]["downstream"] = RowHandle()

    # reset the status
    operator_table[operator]['status'] = EmitterStatus(callback).next_after()


def subscribe_to_relay(relay: RowHandle, subscriber: RowHandle) -> None:
    assert get_app().is_handle_valid(relay)
    assert get_app().is_handle_valid(subscriber)

    relay_table: Table = get_app().get_table(TableIndex.RELAYS)
    assert relay_table[relay]['value'].get_schema() == get_schema(subscriber)

    current_downstream: RowHandleList = relay_table[relay]["downstream"]
    if subscriber not in current_downstream:
        relay_table[relay]["downstream"] = current_downstream.append(subscriber)


def subscribe_to_operator(operator: RowHandle, subscriber: RowHandle) -> None:
    assert get_app().is_handle_valid(operator)
    assert get_app().is_handle_valid(subscriber)

    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    assert operator_table[operator]['value'].get_schema() == get_schema(subscriber)

    current_downstream: RowHandle = operator_table[operator]["downstream"]
    assert current_downstream.is_null()
    operator_table[operator]["downstream"] = subscriber


def get_schema(element: RowHandle) -> Value.Schema:
    assert get_app().is_handle_valid(element)
    if element.table == TableIndex.OPERATORS:
        return get_app().get_table(TableIndex.OPERATORS)[element]['schema']
    else:
        assert element.table == TableIndex.RELAYS
        return get_app().get_table(TableIndex.RELAYS)[element]['value'].get_schema()


def create_relay(initial: Value) -> RowHandle:
    relay_table: Table = get_app().get_table(TableIndex.RELAYS)
    return relay_table.add_row(value=initial)


# APPLICATION ##########################################################################################################

class Application:
    class Data(NamedTuple):
        storage: Storage
        event_loop: EventLoop

    def __init__(self):
        self._data: Optional[Application.Data] = None

    def get_table(self, table_index: Union[int, TableIndex]) -> Table:
        assert self._data
        return self._data.storage[int(table_index)]

    def is_handle_valid(self, handle: RowHandle) -> bool:
        return self.get_table(handle.table).is_handle_valid(handle)

    def schedule_event(self, callback: Callable, *args):
        assert self._data
        self._data.event_loop.schedule((callback, *args))

    def run(self, setup_func: Callable[[Any, Scene], None], root_desc: NodeDescription) -> int:
        # initialize glfw
        if not glfw.init():
            return -1

        # create the application data
        self._data = Application.Data(
            storage=Storage(
                operators=OperatorRow,
                facts=RelayRow,
                nodes=NodeRow,
            ),
            event_loop=EventLoop(),
        )
        scene = Scene(root_desc)

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
        event_thread = Thread(target=self._data.event_loop.run)
        event_thread.start()

        # execute the user-supplied setup function
        setup_func(window, scene)

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
            self._data.event_loop.close()
            event_thread.join()

            nanovg._nanovg.nvgDeleteGLES3(ctx)
            del scene
            self._data = None
            glfw.terminate()

        return 0  # terminated normally


_THE_APPLICATION: Application = Application()


def get_app() -> Application:
    return _THE_APPLICATION


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
    def __init__(self, handle: RowHandle):
        assert (handle.table == TableIndex.OPERATORS)
        self._handle: RowHandle = handle

    @property
    def data(self) -> Value:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["data"]

    def is_valid(self) -> bool:
        return get_app().get_table(TableIndex.OPERATORS).is_handle_valid(self._handle)

    def __getitem__(self, name: str):
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["args"][name]

    def next(self, value: Value) -> None:
        emit_from_operator(self._handle, OperatorCallback.NEXT, value)

    def fail(self, error: Value) -> None:
        emit_from_operator(self._handle, OperatorCallback.FAILURE, error)

    def complete(self) -> None:
        emit_from_operator(self._handle, OperatorCallback.COMPLETION, Value())

    def schedule(self, callback: Callable, *args):
        assert iscoroutinefunction(callback)

        async def update_data_on_completion():
            get_app().get_table(self._handle.table)[self._handle]["data"] = await callback(*args)

        get_app().schedule_event(update_data_on_completion)


class Fact:
    def __init__(self, handle: RowHandle):
        self._handle: RowHandle = handle

    def __del__(self):
        table: Table = get_app().get_table(TableIndex.RELAYS)
        if table.is_handle_valid(self._handle):
            table.remove_row(self._handle)

    def get_value(self) -> Value:
        return get_app().get_table(TableIndex.RELAYS)[self._handle]['value']

    def get_schema(self) -> Value.Schema:
        return self.get_value().get_schema()

    def next(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        get_app().schedule_event(lambda: emit_from_relay(self._handle, OperatorCallback.NEXT, value))

    def fail(self, error: Value) -> None:
        get_app().schedule_event(lambda: emit_from_relay(self._handle, OperatorCallback.FAILURE, error))

    def complete(self) -> None:
        get_app().schedule_event(lambda: emit_from_relay(self._handle, OperatorCallback.COMPLETION, Value()))

    def subscribe(self, downstream: RowHandle):
        subscribe_to_relay(self._handle, downstream)


# SCENE ################################################################################################################

class Scene:

    def __init__(self, root_description: NodeDescription):

        # create the root node
        node_table: Table = get_app().get_table(TableIndex.NODES)
        relay_table: Table = get_app().get_table(TableIndex.RELAYS)
        root_handle: RowHandle = node_table.add_row(
            description=root_description,
            parent=RowHandle(),  # empty row handle as parent
            sockets=NodeSockets(
                names=Bonsai([socket_name for socket_name in root_description.sockets.keys()]),
                elements=[(socket_direction, relay_table.add_row(value=Value(socket_schema))) for
                          (socket_direction, socket_schema) in root_description.sockets.values()],
            )
        )
        self._root_node: Node = Node(root_handle)
        self._root_node.transition_into(root_description.state_machine.initial)

    def __del__(self):
        self._root_node.remove()

    def create_node(self, name: str, description: NodeDescription):
        self._root_node.create_child(name, description)

    def get_relay(self, name: str) -> Optional[RowHandle]:
        path: List[str] = name.split('/')
        path[-1], relay_name = path[-1].split(':', maxsplit=1)
        node: Node = self._root_node
        for step in path:
            node = Node(node.get_child(step))
        return node.get_socket(relay_name)


class Node:
    def __init__(self, handle: RowHandle):
        assert handle.table == TableIndex.NODES
        self._handle: RowHandle = handle

    # TODO: create child and -operator and state transition and all other private node methods should be operators...
    def create_child(self, name: str, description: NodeDescription) -> RowHandle:
        # ensure the child name is unique
        node_table: Table = get_app().get_table(TableIndex.NODES)
        assert name not in node_table[self._handle]['children']

        # create the child node entry
        relay_table: Table = get_app().get_table(TableIndex.RELAYS)
        child_handle: RowHandle = node_table.add_row(
            description=description,
            parent=self._handle,
            sockets=NodeSockets(
                names=Bonsai([socket_name for socket_name in description.sockets.keys()]),
                elements=[(socket_direction, relay_table.add_row(value=Value(socket_schema))) for
                          (socket_direction, socket_schema) in description.sockets.values()],
            )
        )
        node_table[self._handle]['children'] = node_table[self._handle]['children'].set(name, child_handle)

        # initialize the child node by transitioning into its initial state
        child_node: Node = Node(child_handle)
        child_node.transition_into(description.state_machine.initial)

        return child_handle

    def get_name(self) -> str:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if parent_handle.is_null():
            return '<root>'
        children: RowHandleMap = node_table[parent_handle]['children']
        for index, (child_name, child_handle) in enumerate(children.items()):
            if child_handle == self._handle:
                return child_name
        assert False

    def get_child(self, name: str) -> Optional[RowHandle]:
        return get_app().get_table(TableIndex.NODES)[self._handle]['children'].get(name)

    def get_socket(self, name: str) -> Optional[RowHandle]:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        sockets: NodeSockets = node_table[self._handle]['sockets']
        index: Optional[int] = sockets.names.get(name)
        if index is None:
            return None
        return sockets.elements[index][1]
        # TODO: differentiate inputs/outputs? Right now, you can connect an output as downstream from external

    def remove(self):
        # unregister from the parent
        node_table: Table = get_app().get_table(TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if not parent_handle.is_null():
            children: RowHandleMap = node_table[parent_handle]['children']
            children.remove(self.get_name())

        # remove all children first and then yourself
        self._remove_recursively()

    def transition_into(self, state: str) -> None:
        # ensure the transition is allowed
        node_table: Table = get_app().get_table(TableIndex.NODES)
        current_state: str = node_table[self._handle]['state']
        node_description: NodeDescription = node_table[self._handle]['description']
        assert current_state == '' or (current_state, state) in node_description.state_machine.transitions

        # remove the current dynamic network
        self._clear_network()

        # create new elements
        network: List[RowHandle] = []
        network_description: NodeNetworkDescription = node_description.state_machine.states[state]
        for kind, args in network_description.operators:
            network.append(ELEMENT_TABLE[kind][OperatorCallback.CREATE](args))
        node_table[self._handle]['network'] = RowHandleList(network)

        for input_name, operator_index in network_description.incoming_connections:
            relay: RowHandle = self.get_socket(input_name)
            subscribe_to_relay(relay, network[operator_index])

        # TODO: create internal connections (and all others)
        # children = field(type=RowHandleMap, mandatory=True, initial=RowHandleMap())
        # network = field(type=RowHandleList, mandatory=True, initial=RowHandleList())

    def _remove_recursively(self):
        """
        Unlike `remove`, this does not remove this Node from its parent as we know that the parent is also in the
        process of removing itself from the Scene.
        """
        # remove all children
        node_table: Table = get_app().get_table(TableIndex.NODES)
        child_handles: RowHandleMap = node_table[self._handle]['children']
        for child_handle in child_handles.values():
            Node(child_handle)._remove_recursively()

        # remove dynamic network
        self._clear_network()

        # remove sockets
        sockets: NodeSockets = node_table[self._handle]['sockets']
        relay_table: Table = get_app().get_table(TableIndex.RELAYS)
        for _, socket_handle in sockets.elements:
            relay_table.remove_row(socket_handle)

        # remove the node
        node_table.remove_row(self._handle)

    def _clear_network(self) -> None:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        network_handles: RowHandleList = node_table[self._handle]['network']
        operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
        for operator in network_handles:
            operator_table.remove_row(operator)
        node_table[self._handle]['network'] = RowHandleList()


# OPERATOR REGISTRY ####################################################################################################


def create_buffer_operator(args: Value) -> RowHandle:
    schema: Value.Schema = Value.Schema.from_value(args['schema'])
    assert schema
    return get_app().get_table(TableIndex.OPERATORS).add_row(
        value=Value(0),
        op_index=OperatorKind.BUFFER,
        schema=schema,
        args=Value(dict(time_span=args['time_span'])),
        data=Value(dict(is_running=False, counter=0)),
    )


# TODO: this is not actually a buffer...
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


ELEMENT_TABLE: Tuple[
    Tuple[
        Optional[Callable[[Operator, RowHandle], Value]],
        Optional[Callable[[Operator, RowHandle], Value]],
        Optional[Callable[[Operator, RowHandle], Value]],
        Callable[[Value], RowHandle],
    ], ...] = (
    (None, None, None, create_relay),  # relay
    (buffer_on_next, None, None, create_buffer_operator),  # buffer
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

root_node: NodeDescription = NodeDescription(
    sockets={}, state_machine=NodeStateMachine(states=dict(default=NodeNetworkDescription(
        operators=[],
        internal_connections=[],
        external_connections=[],
        incoming_connections=[],
        outgoing_connections=[],
    )), transitions=[], initial='default'))

count_presses_node: NodeDescription = NodeDescription(
    sockets=dict(key_down=(NodeSocketDirection.INPUT, Value.Schema())),
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


# TODO: instead of this function, use the root node description's initial state for setup
def app_setup(window, scene: Scene) -> None:
    def key_callback_fn(_1, key, _2, action, _3):
        nonlocal key_fact
        if action not in (glfw.PRESS, glfw.REPEAT):
            return

        if key == glfw.KEY_ESCAPE:
            glfw.set_window_should_close(window, 1)
            del key_fact  # TODO: Fact will outlive the application when closed through window X
            return

        if glfw.KEY_A <= key <= glfw.KEY_Z:
            # if (mods & glfw.MOD_SHIFT) != glfw.MOD_SHIFT:
            #     key += 32
            # char: str = bytes([key]).decode()
            key_fact.next(Value())

    key_fact: Fact = Fact(create_relay(Value()))
    scene.create_node("herbert", count_presses_node)
    key_fact.subscribe(scene.get_relay('herbert:key_down'))
    glfw.set_key_callback(window, key_callback_fn)


# MAIN #################################################################################################################

if __name__ == "__main__":
    sys.exit(get_app().run(app_setup, root_node))
