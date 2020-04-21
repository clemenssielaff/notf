from __future__ import annotations

import sys
from enum import IntEnum, auto, unique, Enum
from threading import Thread
from typing import Tuple, Callable, Any, Optional, Dict, Union, List, NamedTuple
from types import CodeType
from inspect import iscoroutinefunction
from math import inf

import glfw
import curio
from pyrsistent import field

sys.path.append(r'/home/clemens/code/notf/pynotf')

from pynotf.data import Value, RowHandle, Table, TableRow, Storage, mutate_value, RowHandleList, Bonsai, RowHandleMap
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


class Size2(NamedTuple):
    width: float
    height: float


Xform = Tuple[float, float, float, float, float, float]


@unique
class NodeSocketKind(Enum):
    INPUT = auto()
    PROPERTY = auto()
    OUTPUT = auto()


class NodeNetworkDescription(NamedTuple):  # TODO: assign integers to sockets as well
    operators: List[Tuple[OperatorIndex, Value]]
    internal_connections: List[Tuple[int, int]]  # internal to internal
    external_connections: List[Tuple[str, str]]  # external to input
    incoming_connections: List[Tuple[str, int]]  # input to internal
    outgoing_connections: List[Tuple[int, str]]  # internal to output


NodeDesign = List[Tuple[Any, ...]]


class NodeStateDescription(NamedTuple):
    network: NodeNetworkDescription
    design: NodeDesign


class NodeStateMachine(NamedTuple):
    states: Dict[str, NodeStateDescription]
    transitions: List[Tuple[str, str]]
    initial: str


class NodeSockets(NamedTuple):
    names: Bonsai
    elements: List[Tuple[NodeSocketKind, RowHandle]]


class NodeDescription(NamedTuple):
    sockets: Dict[str, Tuple[NodeSocketKind, Value.Schema]]
    state_machine: NodeStateMachine


class Claim(NamedTuple):
    class Stretch(NamedTuple):
        minimum: float = 0.
        preferred: float = 0.
        maximum: float = inf
        scale_factor: float = 1.
        priority: int = 0

    vertical: Claim.Stretch = Stretch()
    horizontal: Claim.Stretch = Stretch()


class NodeData(NamedTuple):
    opacity: float = 1.
    depth_override: int = 0
    xform_local: Xform = (1, 0, 0, 1, 0, 0)
    claim: Claim = Claim()
    xform_layout: Xform = (1, 0, 0, 1, 0, 0)
    grant: Size2 = Size2(0, 0)


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
    flags = field(type=int, mandatory=True)  # flags and op_index could be stored in the same 64 bit word
    value = field(type=Value, mandatory=True)
    input_schema = field(type=Value.Schema, mandatory=True)
    args = field(type=Value, mandatory=True)
    data = field(type=Value, mandatory=True)
    upstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


class OperatorRowDescription(NamedTuple):
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
    data = field(type=NodeData, mandatory=True, initial=NodeData())
    children = field(type=RowHandleMap, mandatory=True, initial=RowHandleMap())
    network = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# FUNCTIONS ############################################################################################################

def create_operator(data: OperatorRowDescription) -> RowHandle:
    """
    For example, for the Factory operator, we want to inspect the input/output Schema of another, yet-to-be-created
    Operator without actually creating one.
    Therefore, the creator functions only return OperatorRowDescription, that *this* function then turns into an actual row
    in the Operator table.
    :param data: Date from which to construct the new row.
    :return: The handle to the created row.
    """
    return get_app().get_table(TableIndex.OPERATORS).add_row(
        op_index=data.operator_index,
        value=data.initial_value,
        input_schema=data.input_schema,
        args=data.args,
        data=data.data,
        flags=data.flags,
    )


# TODO: all of these getters/setters should be part of an `Operator` class that only has the row handle as field
def get_op_index(handle: RowHandle) -> int:
    assert handle.table == TableIndex.OPERATORS
    return get_app().get_table(TableIndex.OPERATORS)[handle]['op_index']


def get_flags(handle: RowHandle) -> int:
    assert handle.table == TableIndex.OPERATORS
    return get_app().get_table(TableIndex.OPERATORS)[handle]['flags']


def get_status(handle: RowHandle) -> EmitterStatus:
    return EmitterStatus(get_flags(handle) >> FlagIndex.STATUS)


def set_status(handle: RowHandle, status: EmitterStatus) -> None:
    assert handle.table == TableIndex.OPERATORS
    table: Table = get_app().get_table(TableIndex.OPERATORS)
    table[handle]['flags'] = (table[handle]['flags'] & ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)


def is_external(handle: RowHandle) -> bool:
    return bool(get_flags(handle) & (1 << FlagIndex.IS_EXTERNAL))


def is_multicast(handle: RowHandle) -> bool:
    return bool(get_flags(handle) & (1 << FlagIndex.IS_MULTICAST))


def get_input_schema(handle: RowHandle) -> Value.Schema:
    assert handle.table == TableIndex.OPERATORS
    return get_app().get_table(TableIndex.OPERATORS)[handle]['input_schema']


def get_value(handle: RowHandle) -> Value:
    assert handle.table == TableIndex.OPERATORS
    return get_app().get_table(TableIndex.OPERATORS)[handle]['value']


def set_value(handle: RowHandle, value: Value, check_schema: Optional[Value.Schema] = None) -> bool:
    assert handle.table == TableIndex.OPERATORS
    if check_schema is not None and check_schema != value.get_schema():
        return False
    else:
        get_app().get_table(TableIndex.OPERATORS)[handle]['value'] = value
        return True


def get_upstream(handle: RowHandle) -> RowHandleList:
    assert handle.table == TableIndex.OPERATORS
    return get_app().get_table(TableIndex.OPERATORS)[handle]['upstream']


def add_upstream(downstream: RowHandle, upstream: RowHandle) -> None:
    assert downstream.table == upstream.table == TableIndex.OPERATORS
    current_upstream: RowHandleList = get_upstream(downstream)
    if upstream not in current_upstream:
        get_app().get_table(TableIndex.OPERATORS)[downstream]['upstream'] = current_upstream.append(upstream)


def remove_upstream(downstream: RowHandle, upstream: RowHandle) -> bool:
    assert downstream.table == upstream.table == TableIndex.OPERATORS
    table: Table = get_app().get_table(TableIndex.OPERATORS)
    current_upstream: RowHandleList = table[downstream]['upstream']
    if upstream in current_upstream:
        table[downstream]['upstream'] = current_upstream.remove(upstream)
        return True
    else:
        return False


def get_downstream(handle: RowHandle) -> RowHandleList:
    assert handle.table == TableIndex.OPERATORS
    return get_app().get_table(TableIndex.OPERATORS)[handle]['downstream']


def add_downstream(upstream: RowHandle, downstream: RowHandle) -> None:
    assert upstream.table == downstream.table == TableIndex.OPERATORS
    table: Table = get_app().get_table(TableIndex.OPERATORS)
    current_downstream: RowHandleList = get_downstream(upstream)
    if downstream not in current_downstream:
        table[upstream]['downstream'] = current_downstream.append(downstream)


def remove_downstream(upstream: RowHandle, downstream: RowHandle) -> bool:
    assert upstream.table == downstream.table == TableIndex.OPERATORS
    table: Table = get_app().get_table(TableIndex.OPERATORS)
    current_downstream: RowHandleList = table[upstream]['downstream']
    if downstream in current_downstream:
        table[upstream]['downstream'] = current_downstream.remove(downstream)
        return True
    else:
        return False


def run_downstream(operator: RowHandle, source: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert get_app().is_handle_valid(operator)
    assert get_app().is_handle_valid(source)

    # make sure the operator is valid and not completed yet
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    status: EmitterStatus = get_status(operator)
    if status.is_completed():
        return  # operator has completed

    # find the callback to perform
    callback_func: Optional[Callable] = ELEMENT_TABLE[get_op_index(operator)][callback]
    if callback_func is None:
        return  # operator type does not provide the requested callback

    # perform the callback ...
    if callback == OperatorCallback.NEXT:
        if get_input_schema(operator).is_none():
            new_data: Value = callback_func(OperatorSelf(operator), source,
                                            Value())  # for operators with no input value
        else:
            new_data: Value = callback_func(OperatorSelf(operator), source, value)

        # ...and update the operator's data
        assert new_data.get_schema() == operator_table[operator]["data"].get_schema()
        if new_data != operator_table[operator]["data"]:
            operator_table[operator]['data'] = new_data

    else:
        # the failure and completion callbacks do not return a value
        callback_func(OperatorSelf(operator), source, value)


def emit_downstream(operator: RowHandle, callback: OperatorCallback, value: Value) -> None:
    assert callback != OperatorCallback.CREATE

    # make sure the operator is valid and not completed yet
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    status: EmitterStatus = get_status(operator)
    if status.is_completed():
        return  # operator has completed

    # mark the operator as actively emitting
    assert not status.is_active()  # cyclic error
    set_status(operator, EmitterStatus(callback))  # set the active status

    # copy the list of downstream handles, in case it changes during the emission
    downstream: RowHandleList = get_downstream(operator)

    if callback == OperatorCallback.NEXT:

        # store the emitted value and ensure that the operator is able to emit the given value
        set_value(operator, value, get_value(operator).get_schema())

        # emit to all valid downstream elements
        for element in downstream:
            run_downstream(element, operator, callback, value)

        # reset the status
        set_status(operator, EmitterStatus.IDLE)

    else:
        # store the emitted value, bypassing the schema check
        set_value(operator, value)

        # emit one last time ...
        for element in downstream:
            run_downstream(element, operator, callback, value)

        # ... and finalize the status
        set_status(operator, EmitterStatus(int(callback) + 3))

        # unsubscribe all downstream handles (this might destroy the Operator, therefore it is the last thing we do)
        for element in downstream:
            unsubscribe(operator, element)

        # if the Operator is external, it will still be alive after everyone unsubscribed
        if operator_table.is_handle_valid(operator):
            assert is_external(operator)
            assert len(get_downstream(operator)) == 0


def subscribe(upstream: RowHandle, downstream: RowHandle) -> None:
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    assert operator_table.is_handle_valid(upstream)
    assert operator_table.is_handle_valid(downstream)
    assert get_input_schema(downstream).is_none() or (get_value(upstream).get_schema() == get_input_schema(downstream))

    # if the operator upstream has already completed, call the corresponding callback immediately and do not subscribe
    upstream_status: EmitterStatus = get_status(upstream)
    if upstream_status.is_completed():
        assert is_external(upstream)  # operator is valid but completed? -> it must be external
        if upstream_status.is_active():
            return run_downstream(downstream, upstream, OperatorCallback(upstream_status), get_value(upstream))
        else:
            return run_downstream(downstream, upstream, OperatorCallback(int(upstream_status) - 3), get_value(upstream))

    if len(get_downstream(upstream)) != 0:
        assert is_multicast(upstream)

    add_downstream(upstream, downstream)
    add_upstream(downstream, upstream)


def unsubscribe(upstream: RowHandle, downstream: RowHandle) -> None:
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    assert operator_table.is_handle_valid(upstream)
    assert operator_table.is_handle_valid(downstream)

    # TODO: I don't really have a good idea yet how to handle error and completion values.
    #  they are also stored in the `value` field, but that screws with the schema compatibility check
    # assert get_input_schema(downstream).is_none() or (get_value(upstream).get_schema() == get_input_schema(downstream))

    # if the upstream was already completed when the downstream subscribed, its subscription won't have completed
    current_upstream: RowHandleList = get_upstream(downstream)
    if upstream not in current_upstream:
        assert len(get_downstream(upstream)) == 0
        assert get_status(upstream).is_completed()
        return

    # update the downstream element
    remove_upstream(downstream, upstream)

    # update the upstream element
    remove_downstream(upstream, downstream)

    # if the upstream element was internal and this was its last subscriber, remove it
    if len(get_downstream(upstream)) == 0 and not is_external(upstream):
        return remove_operator(upstream)


def remove_operator(operator: RowHandle) -> None:
    operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
    if not operator_table.is_handle_valid(operator):
        return

    # remove from all remaining downstream elements
    for downstream in list(get_downstream(operator)):
        remove_upstream(downstream, operator)

    # unsubscribe from all upstream elements
    for upstream in list(get_upstream(operator)):
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
                with Painterpreter(window, ctx) as painter:
                    painter.paint(scene.get_node("herbert"))
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


# PAINTERPRETER ########################################################################################################

class Properties:
    def __init__(self, node: RowHandle):
        self._handle: RowHandle = node

    def __getitem__(self, name: str) -> Value:
        property_handle: Optional[RowHandle] = Node(self._handle).get_property(name)
        assert property_handle
        return get_value(property_handle)


class Expression:
    def __init__(self, source: str):
        self._source: str = source
        self._code: CodeType = compile(self._source, '<string>', mode='eval')  # might raise a SyntaxError

    def execute(self, scope: Optional[Dict[str, Any]] = None) -> Any:
        return eval(self._code, {}, scope or {})

    def __repr__(self) -> str:
        return f'Expression("{self._source}")'


class Painterpreter:

    def __init__(self, window, context):
        self._window = window
        self._context = context
        self._window_size: Size2 = Size2(*glfw.get_window_size(window))
        self._buffer_size: Size2 = Size2(*glfw.get_framebuffer_size(window))

    def __enter__(self) -> Painterpreter:
        gl.viewport(0, 0, *self._buffer_size)
        gl.clearColor(0.098, 0.098, .098, 1)
        gl.clear(gl.COLOR_BUFFER_BIT, gl.DEPTH_BUFFER_BIT, gl.STENCIL_BUFFER_BIT)

        gl.enable(gl.BLEND)
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA)
        gl.enable(gl.CULL_FACE)
        gl.disable(gl.DEPTH_TEST)

        nanovg.begin_frame(self._context, self._window_size.width, self._window_size.height,
                           self._buffer_size.width / self._window_size.width)

        return self

    def paint(self, node: RowHandle) -> None:
        scope: Dict[str, Any] = dict(
            window=self._buffer_size,
            prop=Properties(node),
        )

        for command in Node(node).get_design():
            func_name: str = command[0]
            assert isinstance(func_name, str)

            args = []
            for arg in command[1:]:
                if isinstance(arg, Expression):
                    args.append(arg.execute(scope))
                else:
                    args.append(arg)

            getattr(nanovg, func_name)(self._context, *args)

    def __exit__(self, exc_type, exc_val, exc_tb):
        nanovg.end_frame(self._context)
        glfw.swap_buffers(self._window)  # TODO: this is happening in the MAIN loop - it should happen on a 3nd thread


# EVENT LOOP ###########################################################################################################

class EventLoop:

    def __init__(self):
        self._events = curio.UniversalQueue()
        self._should_close: bool = False

    def schedule(self, event) -> None:
        self._events.put(event)

    def close(self):
        self._should_close = True
        self._events.put(None)

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
            try:
                task_result: Any = await coroutine(*args)
            except curio.errors.TaskCancelled:
                return None
            finished_tasks.append(await curio.current_task())
            return task_result

        while True:
            event = await self._events.get()

            # close the loop
            if self._should_close:
                for task in active_tasks:
                    await task.cancel()
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

class OperatorSelf:
    """
    Operator access within one of its callback functions.
    """

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
        return get_downstream(self._handle)

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
            op_index=OperatorIndex.RELAY,
            input_schema=initial.get_schema(),
            args=Value(),
            data=Value(),
            flags=create_flags(external=True),
        )

    def __del__(self):
        remove_operator(self._handle)

    def get_value(self) -> Value:
        return get_value(self._handle)

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

    def get_node(self, name: str) -> Optional[RowHandle]:
        path: List[str] = name.split('/')
        node: Node = self._root_node
        for step in path:
            node = Node(node.get_child(step))
        return node.get_handle()

    def get_socket(self, name: str) -> Optional[RowHandle]:
        path, socket = name.rsplit(':', maxsplit=1)
        return Node(self.get_node(path)).get_socket(socket)


class Node:
    def __init__(self, handle: RowHandle):
        assert handle.table == TableIndex.NODES
        self._handle: RowHandle = handle

    def get_handle(self) -> RowHandle:
        return self._handle

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

    def get_property(self, name: str) -> Optional[RowHandle]:
        socket_handle: Optional[RowHandle] = self.get_socket(name)
        if socket_handle is None or get_op_index(socket_handle) != OperatorIndex.PROPERTY:
            return None
        else:
            return socket_handle

    def get_socket(self, name: str) -> Optional[RowHandle]:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        sockets: NodeSockets = node_table[self._handle]['sockets']
        index: Optional[int] = sockets.names.get(name)
        if index is None:
            return None
        socket_handle: RowHandle = sockets.elements[index][1]
        assert socket_handle.table == TableIndex.OPERATORS
        return socket_handle
        # TODO: differentiate inputs/outputs? Right now, you can connect an output as downstream from external

    def get_state(self) -> str:
        return get_app().get_table(TableIndex.NODES)[self._handle]['state']

    def get_design(self) -> NodeDesign:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        return node_table[self._handle]['description'].state_machine.states[self.get_state()].design

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

        # enter the new state
        new_state: NodeStateDescription = node_description.state_machine.states[state]
        node_table[self._handle]['state'] = state

        # create new elements
        network: List[RowHandle] = []
        network_description: NodeNetworkDescription = new_state.network
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
    def create(value: Value) -> OperatorRowDescription:
        return OperatorRowDescription(
            operator_index=OperatorIndex.RELAY,
            initial_value=value,
            input_schema=value.get_schema(),
        )

    @staticmethod
    def on_next(self: OperatorSelf, _: RowHandle, value: Value) -> Value:
        self.next(value)
        return self.data


class OpProperty:
    """
    A Property is basically a Relay with the requirement that it's value schema is not none.
    """

    @staticmethod
    def create(value: Value) -> OperatorRowDescription:
        assert not value.get_schema().is_none()
        return OperatorRowDescription(
            operator_index=OperatorIndex.PROPERTY,
            initial_value=value,
            input_schema=value.get_schema(),
        )


class OpBuffer:
    @staticmethod
    def create(args: Value) -> OperatorRowDescription:
        schema: Value.Schema = Value.Schema.from_value(args['schema'])
        assert schema
        return OperatorRowDescription(
            operator_index=OperatorIndex.BUFFER,
            initial_value=Value(0),
            input_schema=schema,
            args=Value(time_span=args['time_span']),
            data=Value(is_running=False, counter=0),
        )

    @staticmethod
    def on_next(self: OperatorSelf, _upstream: RowHandle, _value: Value) -> Value:
        if not int(self.data["is_running"]) == 1:
            async def timeout():
                await curio.sleep(float(self["time_span"]))
                if not self.is_valid():
                    return
                self.next(Value(self.data["counter"]))
                print(f'Clicked {int(self.data["counter"])} times in the last {float(self["time_span"])} seconds')
                return mutate_value(self.data, False, "is_running")

            self.schedule(timeout)
            return Value(is_running=True, counter=1)

        else:
            return mutate_value(self.data, self.data["counter"] + 1, "counter")


class OpFactory:
    """
    Creates an Operator that takes an empty Signal and produces and subscribes a new Operator for each subscription.
    """

    @staticmethod
    def create(args: Value) -> OperatorRowDescription:
        operator_id: int = int(args['id'])
        factory_arguments: Value = args['args']
        example_operator_data: OperatorRowDescription = ELEMENT_TABLE[operator_id][OperatorCallback.CREATE](
            factory_arguments)

        return OperatorRowDescription(
            operator_index=OperatorIndex.FACTORY,
            initial_value=example_operator_data.initial_value,
            input_schema=Value.Schema(),
            args=args,
        )

    @staticmethod
    def on_next(self: OperatorSelf, _1: RowHandle, _2: Value) -> Value:
        downstream: RowHandleList = self.get_downstream()
        if len(downstream) == 0:
            return self.data

        factory_function: Callable[[Value], OperatorRowDescription] = ELEMENT_TABLE[int(self['id'])][
            OperatorCallback.CREATE]
        factory_arguments: Value = self['args']
        for subscriber in downstream:
            new_operator: RowHandle = create_operator(factory_function(factory_arguments))
            subscribe(new_operator, subscriber)
            run_downstream(new_operator, self.get_handle(), OperatorCallback.NEXT, get_value(new_operator))

        return self.data


class OpCountdown:
    @staticmethod
    def create(args: Value) -> OperatorRowDescription:
        return OperatorRowDescription(
            operator_index=OperatorIndex.COUNTDOWN,
            initial_value=Value(0),
            input_schema=Value.Schema(),
            args=args,
        )

    @staticmethod
    def on_next(self: OperatorSelf, _upstream: RowHandle, _value: Value) -> Value:
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
    def create(value: Value) -> OperatorRowDescription:
        return OperatorRowDescription(
            operator_index=OperatorIndex.PRINTER,
            initial_value=value,
            input_schema=value.get_schema(),
        )

    @staticmethod
    def on_next(self: OperatorSelf, upstream: RowHandle, value: Value) -> Value:
        print(f'Received {value!r} from {upstream}')
        return self.data


# class OpBuffer:
#     @staticmethod
#     def create(args: Value) -> OperatorRowDescription:
#         pass
#
#     @staticmethod
#     def on_next(self: Operator, upstream: RowHandle, value: Value) -> Value:
#         pass

@unique
class OperatorIndex(IndexEnum):
    RELAY = auto()
    PROPERTY = auto()
    BUFFER = auto()
    FACTORY = auto()
    COUNTDOWN = auto()
    PRINTER = auto()


ELEMENT_TABLE: Tuple[
    Tuple[
        Optional[Callable[[OperatorSelf, RowHandle], Value]],
        Optional[Callable[[OperatorSelf, RowHandle], Value]],
        Optional[Callable[[OperatorSelf, RowHandle], Value]],
        Callable[[Value], OperatorRowDescription],
    ], ...] = (
    (OpRelay.on_next, None, None, OpRelay.create),
    (OpRelay.on_next, None, None, OpProperty.create),  # property uses relay's `on_next` function
    (OpBuffer.on_next, None, None, OpBuffer.create),
    (OpFactory.on_next, None, None, OpFactory.create),
    (OpCountdown.on_next, None, None, OpCountdown.create),
    (OpPrinter.on_next, None, None, OpPrinter.create),
)

# SETUP ################################################################################################################

root_node: NodeDescription = NodeDescription(
    sockets={}, state_machine=NodeStateMachine(
        states=dict(
            default=NodeStateDescription(
                network=NodeNetworkDescription(
                    operators=[],
                    internal_connections=[],
                    external_connections=[],
                    incoming_connections=[],
                    outgoing_connections=[],
                ),
                design=[],
            )
        ),
        transitions=[],
        initial='default'
    )
)

count_presses_node: NodeDescription = NodeDescription(
    sockets=dict(key_down=(NodeSocketKind.INPUT, Value.Schema())),
    state_machine=NodeStateMachine(
        states=dict(
            default=NodeStateDescription(
                network=NodeNetworkDescription(
                    operators=[
                        (OperatorIndex.BUFFER, Value(schema=Value.Schema(), time_span=1)),
                        (OperatorIndex.PRINTER, Value()),
                    ],
                    internal_connections=[
                        (0, 1),
                    ],
                    external_connections=[],
                    incoming_connections=[
                        ('key_down', 0),
                    ],
                    outgoing_connections=[],
                ),
                design=[],
            ),
        ),
        transitions=[],
        initial='default'
    )
)

countdown_node: NodeDescription = NodeDescription(
    sockets=dict(key_down=(NodeSocketKind.INPUT, Value.Schema())),
    state_machine=NodeStateMachine(
        states=dict(
            default=NodeStateDescription(
                network=NodeNetworkDescription(
                    operators=[
                        (OperatorIndex.FACTORY, Value(id=OperatorIndex.COUNTDOWN, args=Value(start=5))),
                        (OperatorIndex.PRINTER, Value(0)),
                    ],
                    internal_connections=[
                        (0, 1),
                    ],
                    external_connections=[],
                    incoming_connections=[
                        ('key_down', 0),
                    ],
                    outgoing_connections=[],
                ),
                design=[
                    ("begin_path",),
                    ("rounded_rect", 50, 50,
                     Expression('max(0, window.width - 100)'),
                     Expression('max(0, window.height - 100)'), 100),
                    ("fill_color", nanovg.rgba(.5, .5, .5)),
                    ("fill",),
                ],
            ),
        ),
        transitions=[],
        initial='default'
    )
)

# TODO: continue here - try drawing with access to a Property of the herbert countdown node


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
    key_fact.subscribe(scene.get_socket('herbert:key_down'))
    glfw.set_key_callback(window, key_callback_fn)


# MAIN #################################################################################################################

if __name__ == "__main__":
    sys.exit(get_app().run(app_setup, root_node))
