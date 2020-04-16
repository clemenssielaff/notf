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
class TableIndex(IndexEnum):
    OPERATORS = auto()
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


class FlagIndex(IntEnum):
    IS_EXTERNAL = 0  # if this Operator is owned externally, meaning it is destroyed explicitly at some point
    IS_MULTICAST = 1  # if this Operator allows more than one downstream subscriber
    STATUS = 2  # offset of the EmitterStatus


def create_flags(external: bool = False, multicast: bool = False, status: EmitterStatus = EmitterStatus.IDLE) -> int:
    return (((int(external) << FlagIndex.IS_EXTERNAL) |
             (int(multicast) << FlagIndex.IS_MULTICAST)) &
            ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)


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


"""
I had a long long about splitting this table up into several tables, as not all Operators require access to all fields.
The Relay, for example, only requires a Schema, a status and the downstream. And many other operators might not require
the data field. 
However, I have finally decided to use a single table (for now, and maybe ever). The reason is that our goal is to keep
memory local. If we have a table with wide rows, the jumps between each row is large. However, if you were to jump 
between tables, the distances would be much larger. You'd save memory overall, but the access pattern is worse.
Also, having everything in a single table means we don't have to have special cases for different tables (emission from
a table with a list to store the downstream VS. emission from a table that only has a single downstream entry etc.).

If the Operator table is too wide (the data in a single row is large enough so you have a lot of cache misses when you 
jump around in the table), we could store a Box<T> as value instead. This would keep the table itself small (a lot 
smaller than it currently is), but you'd have a jump to random memory whenever you access a row, which might be worse...
"""


class OperatorRow(TableRow):
    __table_index__: int = TableIndex.OPERATORS
    op_index = field(type=int, mandatory=True)
    value = field(type=Value, mandatory=True)
    schema = field(type=Value.Schema, mandatory=True)  # input schema
    args = field(type=Value, mandatory=True)
    data = field(type=Value, mandatory=True)
    flags = field(type=int, mandatory=True)
    upstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


class OperatorRowData(NamedTuple):
    operator_index: int
    initial_value: Value
    input_schema: Value.Schema
    args: Value = Value()
    data: Value = Value()
    flags: int = create_flags()


class NodeRow(TableRow):
    __table_index__: int = TableIndex.NODES
    description = field(type=NodeDescription, mandatory=True)
    parent = field(type=RowHandle, mandatory=True)
    sockets = field(type=NodeSockets, mandatory=True)
    state = field(type=str, mandatory=True, initial='')
    children = field(type=RowHandleMap, mandatory=True, initial=RowHandleMap())
    network = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# FUNCTIONS ############################################################################################################

def create_operator(data: OperatorRowData) -> RowHandle:
    """
    For example, for the Factory operator, we want to inspect the input/output Schema of another, yet-to-be-created
    Operator without actually creating one.
    Therefore, the creator functions only return OperatorRowData, that *this* function then turns into an actual row
    in the Operator table.
    :param data: Date from which to construct the new row.
    :return: The handle to the created row.
    """
    return get_app().get_table(TableIndex.OPERATORS).add_row(
        op_index=data.operator_index,
        value=data.initial_value,
        schema=data.input_schema,
        args=data.args,
        data=data.data,
        flags=data.flags,
    )


def get_status(table: Table, handle: RowHandle) -> EmitterStatus:
    return EmitterStatus(table[handle]['flags'] >> FlagIndex.STATUS)


def set_status(table: Table, handle: RowHandle, status: EmitterStatus) -> None:
    table[handle]['flags'] = (table[handle]['flags'] & ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)


def is_external(flags: int) -> bool:
    return bool(flags & (1 << FlagIndex.IS_EXTERNAL))


def is_multicast(flags: int) -> bool:
    return bool(flags & (1 << FlagIndex.IS_MULTICAST))


def run_downstream(operator: RowHandle, source: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert get_app().is_handle_valid(operator)
    assert get_app().is_handle_valid(source)

    # make sure the operator is valid and not completed yet
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    status: EmitterStatus = get_status(operator_table, operator)
    if status.is_completed():
        return  # operator has completed

    # find the callback to perform
    callback_func: Optional[Callable] = ELEMENT_TABLE[operator_table[operator]["op_index"]][callback]
    if callback_func is None:
        return  # operator type does not provide the requested callback

    # perform the callback ...
    if callback == OperatorCallback.NEXT:
        if operator_table[operator]['schema'].is_none():
            new_data: Value = callback_func(Operator(operator), source,
                                            Value())  # ignore values for operator taking none
        else:
            new_data: Value = callback_func(Operator(operator), source, value)

        # ...and update the operator's data
        assert new_data.get_schema() == operator_table[operator]["data"].get_schema()
        if new_data != operator_table[operator]["data"]:
            operator_table[operator]['data'] = new_data

    else:
        # the failure and completion callbacks do not return a value
        callback_func(Operator(operator), source, value)


def emit_downstream(operator: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert callback != OperatorCallback.CREATE

    # make sure the operator is valid and not completed yet
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    status: EmitterStatus = get_status(operator_table, operator)
    if status.is_completed():
        return  # operator has completed

    # mark the operator as actively emitting
    assert not status.is_active()  # cyclic error
    set_status(operator_table, operator, EmitterStatus(callback))  # set the active status

    # copy the list of downstream handles, in case it changes during the emission
    downstream: RowHandleList = operator_table[operator]["downstream"]

    if callback == OperatorCallback.NEXT:

        # ensure that the operator is able to emit the given value
        assert value.get_schema() == operator_table[operator]['value'].get_schema()

        # store the emitted value
        operator_table[operator]['value'] = value

        # emit to all valid downstream elements
        for element in downstream:
            run_downstream(element, operator, callback, value)

        # reset the status
        set_status(operator_table, operator, EmitterStatus.IDLE)

    else:
        # store the emitted value, bypassing the schema check
        operator_table[operator]['value'] = value

        # emit one last time ...
        for element in downstream:
            run_downstream(element, operator, callback, value)

        # ... and finalize the status
        set_status(operator_table, operator, EmitterStatus(int(callback) + 3))

        # unsubscribe all downstream handles (this might destroy the Operator, therefore it is the last thing we do)
        for element in downstream:
            unsubscribe(operator, element)

        # if the Operator is external, it will still be alive after everyone unsubscribed
        if operator_table.is_handle_valid(operator):
            assert is_external(operator_table[operator]['flags'])
            assert len(operator_table[operator]["downstream"]) == 0


def subscribe(upstream: RowHandle, downstream: RowHandle) -> None:
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    assert operator_table.is_handle_valid(upstream)
    assert operator_table.is_handle_valid(downstream)
    assert operator_table[downstream]['schema'].is_none() or (
            operator_table[upstream]['value'].get_schema() == operator_table[downstream]['schema'])

    # if the operator upstream has already completed, call the corresponding callback immediately and do not subscribe
    upstream_status: EmitterStatus = get_status(operator_table, upstream)
    if upstream_status.is_completed():
        assert is_external(operator_table[upstream]['flags'])  # operator is valid but completed? -> it must be external
        if upstream_status.is_active():
            return run_downstream(
                downstream, upstream, OperatorCallback(upstream_status), operator_table[upstream]['value'])
        else:
            return run_downstream(
                downstream, upstream, OperatorCallback(int(upstream_status) - 3), operator_table[upstream]['value'])

    current_downstream: RowHandleList = operator_table[upstream]["downstream"]
    if len(current_downstream) != 0:
        assert is_multicast(operator_table[upstream]["flags"])

    if downstream not in current_downstream:
        operator_table[upstream]["downstream"] = current_downstream.append(downstream)

    current_upstream: RowHandleList = operator_table[downstream]["upstream"]
    if upstream not in current_upstream:
        operator_table[downstream]["upstream"] = current_upstream.append(upstream)


def unsubscribe(upstream: RowHandle, downstream: RowHandle) -> None:
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    assert operator_table.is_handle_valid(upstream)
    assert operator_table.is_handle_valid(downstream)

    # TODO: I don't really have a good idea yet how to handle error and completion values.
    #  they are also stored in the `value` field, but that screws with the schema compatibility check
    # assert operator_table[downstream]['schema'].is_none() or (
    #         operator_table[upstream]['value'].get_schema() == operator_table[downstream]['schema'])

    # if the upstream was already completed when the downstream subscribed, its subscription won't have completed
    current_upstream: RowHandleList = operator_table[downstream]["upstream"]
    if upstream not in current_upstream:
        assert len(operator_table[upstream]["downstream"]) == 0
        assert get_status(operator_table, upstream).is_completed()
        return

    # update the downstream element
    operator_table[downstream]["upstream"] = current_upstream.remove(upstream)

    # update the upstream element
    current_downstream: RowHandleList = operator_table[upstream]["downstream"]
    assert downstream in current_downstream
    operator_table[upstream]["downstream"] = current_downstream.remove(downstream)

    # if the upstream element was internal and this was its last subscriber, remove it
    if len(current_downstream) == 1 and not is_external(operator_table[upstream]["flags"]):
        return remove_operator(upstream)


def remove_operator(operator: RowHandle) -> None:
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    if not operator_table.is_handle_valid(operator):
        return

    # unsubscribe from all remaining downstream elements
    for downstream in operator_table[operator]["downstream"]:
        operator_table[downstream]["upstream"] = operator_table[downstream]["upstream"].remove(operator)

    # unsubscribe from all upstream elements
    for upstream in operator_table[operator]["upstream"]:
        unsubscribe(upstream, operator)  # this might remove upstream elements

    # finally, remove yourself
    operator_table.remove_row(operator)


def remove_node(node: RowHandle) -> None:
    node_table: Table = get_app().get_table(TableIndex.NODES)
    if node_table.is_handle_valid(node):
        node_table.remove_row(node)


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

    def get_handle(self) -> RowHandle:
        return self._handle

    def get_downstream(self) -> RowHandleList:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["downstream"]

    def __getitem__(self, name: str):
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["args"][name]

    def next(self, value: Value) -> None:
        emit_downstream(self._handle, OperatorCallback.NEXT, value)

    def fail(self, error: Value) -> None:
        emit_downstream(self._handle, OperatorCallback.FAILURE, error)

    def complete(self) -> None:
        emit_downstream(self._handle, OperatorCallback.COMPLETION, Value())

    def schedule(self, callback: Callable, *args):
        assert iscoroutinefunction(callback)

        async def update_data_on_completion():
            result: Optional[Value] = await callback(*args)

            # only update the operator data if it has not completed
            operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
            if operator_table.is_handle_valid(self._handle):
                operator_table[self._handle]["data"] = result

        get_app().schedule_event(update_data_on_completion)


class Fact:
    def __init__(self, initial: Value):
        self._handle: RowHandle = get_app().get_table(TableIndex.OPERATORS).add_row(
            value=initial,
            op_index=OperatorKind.RELAY,
            schema=initial.get_schema(),
            args=Value(),
            data=Value(),
            flags=create_flags(external=True),
        )

    def __del__(self):
        remove_operator(self._handle)

    def get_value(self) -> Value:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]['value']

    def get_schema(self) -> Value.Schema:
        return self.get_value().get_schema()

    def next(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        get_app().schedule_event(lambda: emit_downstream(self._handle, OperatorCallback.NEXT, value))

    def fail(self, error: Value) -> None:
        get_app().schedule_event(lambda: emit_downstream(self._handle, OperatorCallback.FAILURE, error))

    def complete(self) -> None:
        get_app().schedule_event(lambda: emit_downstream(self._handle, OperatorCallback.COMPLETION, Value()))

    def subscribe(self, downstream: RowHandle):
        subscribe(self._handle, downstream)


# SCENE ################################################################################################################

class Scene:

    def __init__(self, root_description: NodeDescription):
        # create the root node
        node_table: Table = get_app().get_table(TableIndex.NODES)
        root_handle: RowHandle = node_table.add_row(
            description=root_description,
            parent=RowHandle(),  # empty row handle as parent
            sockets=NodeSockets(
                names=Bonsai([socket_name for socket_name in root_description.sockets.keys()]),
                elements=[(socket_direction, create_operator(OpRelay.create(Value(socket_schema)))) for
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
        child_handle: RowHandle = node_table.add_row(
            description=description,
            parent=self._handle,
            sockets=NodeSockets(
                names=Bonsai([socket_name for socket_name in description.sockets.keys()]),
                elements=[(socket_direction, create_operator(OpRelay.create(Value(socket_schema)))) for
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
            network.append(create_operator(ELEMENT_TABLE[kind][OperatorCallback.CREATE](args)))
        node_table[self._handle]['network'] = RowHandleList(network)

        # incoming connections
        for input_name, operator_index in network_description.incoming_connections:
            relay: RowHandle = self.get_socket(input_name)
            subscribe(relay, network[operator_index])

        # create internal connections
        for source_index, target_index in network_description.internal_connections:
            subscribe(network[source_index], network[target_index])

        # TODO: create all other connections

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
        for _, socket_handle in sockets.elements:
            remove_operator(socket_handle)

        # remove the node
        remove_node(self._handle)

    def _clear_network(self) -> None:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        for operator in node_table[self._handle]['network']:
            remove_operator(operator)
        node_table[self._handle]['network'] = RowHandleList()


# OPERATOR REGISTRY ####################################################################################################

# TODO: Buffer is not really a Buffer, Countdown is way to specific, Create should utilize its input value etc.

class OpRelay:
    @staticmethod
    def create(value: Value) -> OperatorRowData:
        return OperatorRowData(
            operator_index=OperatorKind.RELAY,
            initial_value=value,
            input_schema=value.get_schema(),
        )

    @staticmethod
    def on_next(self: Operator, _: RowHandle, value: Value) -> Value:
        self.next(value)
        return self.data


class OpBuffer:
    @staticmethod
    def create(args: Value) -> OperatorRowData:
        schema: Value.Schema = Value.Schema.from_value(args['schema'])
        assert schema
        return OperatorRowData(
            operator_index=OperatorKind.BUFFER,
            initial_value=Value(0),
            input_schema=schema,
            args=Value(time_span=args['time_span']),
            data=Value(is_running=False, counter=0),
        )

    @staticmethod
    def on_next(self: Operator, _upstream: RowHandle, _value: Value) -> Value:
        if not int(self.data["is_running"]) == 1:
            async def timeout():
                await curio.sleep(float(self["time_span"]))
                if not self.is_valid():
                    return
                self.next(Value(self.data["counter"]))
                print(f'Clicked {int(self.data["counter"])} times in the last {float(self["time_span"])} seconds')
                return set_value(self.data, False, "is_running")

            self.schedule(timeout)
            return Value(is_running=True, counter=1)

        else:
            return set_value(self.data, self.data["counter"] + 1, "counter")


class OpFactory:
    """
    Creates an Operator that takes an empty Signal and produces and subscribes a new Operator for each subscription.
    """

    @staticmethod
    def create(args: Value) -> OperatorRowData:
        operator_id: int = int(args['id'])
        factory_arguments: Value = args['args']
        example_operator_data: OperatorRowData = ELEMENT_TABLE[operator_id][OperatorCallback.CREATE](factory_arguments)

        return OperatorRowData(
            operator_index=OperatorKind.FACTORY,
            initial_value=example_operator_data.initial_value,
            input_schema=Value.Schema(),
            args=args,
        )

    @staticmethod
    def on_next(self: Operator, _1: RowHandle, _2: Value) -> Value:
        downstream: RowHandleList = self.get_downstream()
        if len(downstream) == 0:
            return self.data

        factory_function: Callable[[Value], OperatorRowData] = ELEMENT_TABLE[int(self['id'])][OperatorCallback.CREATE]
        factory_arguments: Value = self['args']
        for subscriber in downstream:
            new_operator: RowHandle = create_operator(factory_function(factory_arguments))
            subscribe(new_operator, subscriber)
            run_downstream(new_operator, self.get_handle(), OperatorCallback.NEXT,
                           get_app().get_table(TableIndex.OPERATORS)[new_operator]['value'])

        return self.data


class OpCountdown:
    @staticmethod
    def create(args: Value) -> OperatorRowData:
        return OperatorRowData(
            operator_index=OperatorKind.COUNTDOWN,
            initial_value=Value(0),
            input_schema=Value.Schema(),
            args=args,
        )

    @staticmethod
    def on_next(self: Operator, _upstream: RowHandle, _value: Value) -> Value:
        counter: Value = self['start']
        assert counter.get_kind() == Value.Kind.NUMBER

        async def loop():
            nonlocal counter
            self.next(counter)
            while counter > 0:
                counter = counter - 1
                await curio.sleep(1)
                self.next(counter)
            self.complete()

        self.schedule(loop)
        return self.data


class OpPrinter:
    @staticmethod
    def create(value: Value) -> OperatorRowData:
        return OperatorRowData(
            operator_index=OperatorKind.PRINTER,
            initial_value=value,
            input_schema=value.get_schema(),
        )

    @staticmethod
    def on_next(self: Operator, upstream: RowHandle, value: Value) -> Value:
        print(f'Received {value!r} from {upstream}')
        return self.data


# class OpBuffer:
#     @staticmethod
#     def create(args: Value) -> OperatorRowData:
#         pass
#
#     @staticmethod
#     def on_next(self: Operator, upstream: RowHandle, value: Value) -> Value:
#         pass

@unique
class OperatorKind(IndexEnum):
    RELAY = auto()
    BUFFER = auto()
    FACTORY = auto()
    COUNTDOWN = auto()
    PRINTER = auto()


ELEMENT_TABLE: Tuple[
    Tuple[
        Optional[Callable[[Operator, RowHandle], Value]],
        Optional[Callable[[Operator, RowHandle], Value]],
        Optional[Callable[[Operator, RowHandle], Value]],
        Callable[[Value], OperatorRowData],
    ], ...] = (
    (OpRelay.on_next, None, None, OpRelay.create),
    (OpBuffer.on_next, None, None, OpBuffer.create),
    (OpFactory.on_next, None, None, OpFactory.create),
    (OpCountdown.on_next, None, None, OpCountdown.create),
    (OpPrinter.on_next, None, None, OpPrinter.create),
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
                operators=[
                    (OperatorKind.BUFFER, Value(schema=Value.Schema(), time_span=1)),
                    (OperatorKind.PRINTER, Value()),
                ],
                internal_connections=[
                    (0, 1),
                ],
                external_connections=[],
                incoming_connections=[
                    ('key_down', 0),
                ],
                outgoing_connections=[],
            )
        ),
        transitions=[],
        initial='default'
    )
)

countdown_node: NodeDescription = NodeDescription(
    sockets=dict(key_down=(NodeSocketDirection.INPUT, Value.Schema())),
    state_machine=NodeStateMachine(
        states=dict(
            default=NodeNetworkDescription(
                operators=[
                    (OperatorKind.FACTORY, Value(id=OperatorKind.COUNTDOWN, args=Value(start=5))),
                    (OperatorKind.PRINTER, Value(0)),
                ],
                internal_connections=[
                    (0, 1),
                ],
                external_connections=[],
                incoming_connections=[
                    ('key_down', 0),
                ],
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

    key_fact: Fact = Fact(Value())
    # scene.create_node("herbert", count_presses_node)
    scene.create_node("herbert", countdown_node)
    key_fact.subscribe(scene.get_relay('herbert:key_down'))
    glfw.set_key_callback(window, key_callback_fn)


# MAIN #################################################################################################################

if __name__ == "__main__":
    sys.exit(get_app().run(app_setup, root_node))
