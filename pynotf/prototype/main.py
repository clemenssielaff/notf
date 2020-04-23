from __future__ import annotations

import sys
from enum import IntEnum, auto, unique, Enum
from threading import Thread
from typing import Tuple, Callable, Any, Optional, Dict, Union, List, NamedTuple
from types import CodeType
from inspect import iscoroutinefunction
from math import inf, sin, pi
from time import monotonic

import glfw
import curio
from pyrsistent import field

sys.path.append(r'/home/clemens/code/notf/pynotf')

from pynotf.data import Value, RowHandle, Table, TableRow, Storage, mutate_value, RowHandleList, Bonsai, RowHandleMap, \
    Path
import pynotf.extra.pynanovg as nanovg
import pynotf.extra.opengl as gl

from prototype.event_loop import EventLoop


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
    SUBSCRIPTION = 3
    CREATE = 4


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
    SOURCE = auto()
    PROPERTY = auto()
    SINK = auto()


NodeDesign = List[Tuple[Any, ...]]


class NodeStateDescription(NamedTuple):
    operators: Dict[str, Tuple[OperatorIndex, Value]]
    connections: List[Tuple[Path, Path]]
    design: NodeDesign
    children: Dict[str, NodeDescription]


class NodeSockets(NamedTuple):
    names: Bonsai
    elements: List[Tuple[NodeSocketKind, RowHandle]]


class NodeDescription(NamedTuple):
    sockets: Dict[str, Tuple[NodeSocketKind, Value]]
    states: Dict[str, NodeStateDescription]
    transitions: List[Tuple[str, str]]
    initial_state: str


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
    input_schema: Value.Schema = Value.Schema()
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
    # We allow access to the operator data using `self.XXX`. Ensure that a data name is not shadowed by a OperatorSelf
    # method. This is an artifact of the Python language and generally nothing to worry about.
    assert len(set(data.data.get_keys()).intersection(set(dir(OperatorSelf)))) == 0

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

    # execute the upstream's `on_subscribe` callback
    on_subscribe_func: Optional[Callable] = ELEMENT_TABLE[get_op_index(upstream)][OperatorCallback.SUBSCRIPTION]
    if on_subscribe_func is not None:
        on_subscribe_func(OperatorSelf(upstream), downstream)


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

    def __init__(self):
        self._storage: Storage = Storage(
            operators=OperatorRow,
            nodes=NodeRow,
        )
        self._event_loop: EventLoop = EventLoop()
        self._scene: Scene = Scene()

    def get_table(self, table_index: Union[int, TableIndex]) -> Table:
        return self._storage[int(table_index)]

    def is_handle_valid(self, handle: RowHandle) -> bool:
        return self.get_table(handle.table).is_handle_valid(handle)

    def get_scene(self) -> Scene:
        return self._scene

    @staticmethod
    def redraw():
        glfw.post_empty_event()

    def schedule_event(self, callback: Callable, *args):
        self._event_loop.schedule((callback, *args))

    def run(self, root_desc: NodeDescription) -> int:
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
        #  also, where _do_ facts live? Right now, they live as source sockets on the root node...
        glfw.set_key_callback(window, key_callback)
        glfw.set_mouse_button_callback(window, mouse_button_callback)

        # make the window's context current
        glfw.make_context_current(window)
        nanovg._nanovg.loadGLES2Loader(glfw._glfw.glfwGetProcAddress)
        ctx = nanovg._nanovg.nvgCreateGLES3(5)

        # start the event loop
        event_thread = Thread(target=self._event_loop.run)
        event_thread.start()

        try:
            while not glfw.window_should_close(window):
                with Painterpreter(window, ctx) as painter:
                    painter.paint(self._scene.get_node(Path("/herbert")))
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


# LOGIC ################################################################################################################

class OperatorSelf:
    """
    Operator access within one of its callback functions.
    """

    def __init__(self, handle: RowHandle):
        assert (handle.table == TableIndex.OPERATORS)
        self._handle: RowHandle = handle

    def __getitem__(self, name: str) -> Value:
        """
        self['arg'] is the access to the Operator's arguments.
        :param name: Name of the argument to access.
        :return: The requested argument.
        :raise: KeyError
        """
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["args"][name]

    def __getattr__(self, name: str):
        """
        `self.name` is the access to the Operator's private data.
        :param name: Name of the data field to access.
        :return: The requested data field.
        :raise: KeyError.
        """
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["data"][name]

    @property
    def data(self) -> Value:
        """
        Complete data field, for example if you want to return the current data unchanged
        :return:
        """
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["data"]

    def is_valid(self) -> bool:
        return get_app().get_table(TableIndex.OPERATORS).is_handle_valid(self._handle)

    def get_handle(self) -> RowHandle:
        return self._handle

    def get_downstream(self) -> RowHandleList:
        return get_downstream(self._handle)

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
    def __init__(self, handle: RowHandle):
        self._handle: RowHandle = handle

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

    def __init__(self):
        self._root_node: Optional[Node] = None

    def initialize(self, root_description: NodeDescription):
        for kind, _ in root_description.sockets.values():
            assert kind == NodeSocketKind.SOURCE
        facts: Dict[str, OperatorRowDescription] = {
            key: OpRelay.create(value) for key, (_, value) in root_description.sockets.items()
        }  # TODO: shouldn't Facts be external?

        # create the root node
        node_table: Table = get_app().get_table(TableIndex.NODES)
        root_handle: RowHandle = node_table.add_row(
            description=root_description,
            parent=RowHandle(),  # empty row handle as parent
            sockets=NodeSockets(
                names=Bonsai([socket_name for socket_name in root_description.sockets.keys()]),
                elements=[(NodeSocketKind.SOURCE, create_operator(fact)) for fact in facts.values()],
            )
        )
        self._root_node = Node(root_handle)
        self._root_node.transition_into(root_description.initial_state)

    def clear(self):
        if self._root_node:
            self._root_node.remove()

    def create_node(self, name: str, description: NodeDescription) -> RowHandle:
        return self._root_node.create_child(name, description)

    def get_fact(self, name: str, ) -> Optional[Fact]:
        fact_handle: Optional[RowHandle] = self._root_node.get_source(name)
        if fact_handle:
            return Fact(fact_handle)
        else:
            return None

    def get_node(self, path: Path) -> Optional[RowHandle]:
        return self._root_node.get_child(path)

    def get_source(self, name: str) -> Optional[RowHandle]:
        return self._root_node.get_source(name)


class Node:
    def __init__(self, handle: RowHandle):
        assert handle.table == TableIndex.NODES
        self._handle: RowHandle = handle

    def get_handle(self) -> RowHandle:
        return self._handle

    def get_parent(self) -> Optional[RowHandle]:
        return get_app().get_table(TableIndex.NODES)[self._handle]['parent']

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

        # initialize the child node by transitioning into its initial_state state
        child_node: Node = Node(child_handle)
        child_node.transition_into(description.initial_state)

        return child_handle

    def get_name(self) -> str:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if parent_handle.is_null():
            return '/'
        children: RowHandleMap = node_table[parent_handle]['children']
        for index, (child_name, child_handle) in enumerate(children.items()):
            if child_handle == self._handle:
                return child_name
        assert False

    def get_child(self, path: Path, step: int = 0) -> Optional[RowHandle]:
        if step == len(path):
            return self._handle

        # next step is up
        next_step: str = path[step]
        if next_step == Path.STEP_UP:
            parent: Optional[RowHandle] = self.get_parent()
            if parent is None:
                return None
            return Node(parent).get_child(path, step + 1)

        # next step is down
        node_table: Table = get_app().get_table(TableIndex.NODES)
        child_handle: Optional[RowHandle] = node_table[self._handle]['children'].get(next_step)
        if child_handle is None:
            return None
        return Node(child_handle).get_child(path, step + 1)

    def _get_socket(self, name: str) -> Optional[Tuple[NodeSocketKind, RowHandle]]:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        sockets: NodeSockets = node_table[self._handle]['sockets']
        index: Optional[int] = sockets.names.get(name)
        if index is None:
            return None
        return sockets.elements[index]

    def get_source(self, name: str) -> Optional[RowHandle]:
        socket_kind, socket_handle = self._get_socket(name)
        if socket_kind == NodeSocketKind.SOURCE:
            return socket_handle
        else:
            return None

    def get_property(self, name: str) -> Optional[RowHandle]:
        socket_kind, socket_handle = self._get_socket(name)
        if socket_kind == NodeSocketKind.PROPERTY:
            return socket_handle
        else:
            return None

    def get_sink(self, name: str) -> Optional[RowHandle]:
        socket_kind, socket_handle = self._get_socket(name)
        if socket_kind == NodeSocketKind.SINK:
            return socket_handle
        else:
            return None

    def get_state(self) -> str:
        return get_app().get_table(TableIndex.NODES)[self._handle]['state']

    def get_design(self) -> NodeDesign:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        return node_table[self._handle]['description'].states[self.get_state()].design

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
        assert current_state == '' or (current_state, state) in node_description.transitions

        # remove the dynamic operators from last state
        self._clear_operators()
        # TODO: remove children, or think of a better waz to transition between states

        # enter the new state
        node_table[self._handle]['state'] = state

        # create new child nodes
        new_state: NodeStateDescription = node_description.states[state]
        for name, child_description in new_state.children.items():
            self.create_child(name, child_description)

        # create new operators
        network: Dict[str, RowHandle] = {}
        for name, (kind, args) in new_state.operators.items():
            assert name not in network
            network[name] = create_operator(ELEMENT_TABLE[kind][OperatorCallback.CREATE](args))
        node_table[self._handle]['network'] = RowHandleList(network.values())

        def find_operator(path: Path) -> Optional[RowHandle]:
            if path.is_node_path():
                assert path.is_relative() and len(path) == 1  # interpret single name paths as dynamic operator
                return network.get(path[0])

            else:
                assert path.is_leaf_path()
                node_handle: Optional[RowHandle]
                if path.is_absolute():
                    node_handle = get_app().get_scene().get_node(path)
                else:
                    assert path.is_relative()
                    node_handle = self.get_child(path)
                if node_handle:
                    result: Optional[Tuple[NodeSocketKind, RowHandle]] = Node(node_handle)._get_socket(path.get_leaf())
                    if result:
                        return result[1]
            return None

        # create new connections
        for source, sink in new_state.connections:
            subscribe(find_operator(source), find_operator(sink))

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
        self._clear_operators()

        # remove sockets
        sockets: NodeSockets = node_table[self._handle]['sockets']
        for _, socket_handle in sockets.elements:
            remove_operator(socket_handle)

        # remove the node
        remove_node(self._handle)

    def _clear_operators(self) -> None:
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
        if not int(self.is_running) == 1:
            async def timeout():
                await curio.sleep(float(self["time_span"]))
                if not self.is_valid():
                    return
                self.next(Value(self.counter))
                print(f'Clicked {int(self.counter)} times in the last {float(self["time_span"])} seconds')
                return mutate_value(self.data, False, "is_running")

            self.schedule(timeout)
            return Value(is_running=True, counter=1)

        else:
            return mutate_value(self.data, self.counter + 1, "counter")


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


class OpSine:
    @staticmethod
    def create(args: Value) -> OperatorRowDescription:
        frequency: float = 0.5
        amplitude: float = 100
        samples: float = 60  # samples per second
        keys: List[str] = args.get_keys()
        if 'frequency' in keys:
            frequency = float(args['frequency'])
        if 'amplitude' in keys:
            amplitude = float(args['amplitude'])
        if 'samples' in keys:
            amplitude = float(args['samples'])

        return OperatorRowDescription(
            operator_index=OperatorIndex.SINE,
            initial_value=Value(0),
            args=Value(
                frequency=frequency,
                amplitude=amplitude,
                samples=samples,
            )
        )

    @staticmethod
    def on_subscribe(self: OperatorSelf, _1: RowHandle) -> None:
        frequency: float = float(self['frequency'])
        amplitude: float = float(self['amplitude'])
        samples: float = float(self['samples'])

        async def runner():
            while self.is_valid():
                self.next(Value((sin(2 * pi * frequency * monotonic()) + 1) * amplitude * 0.5))
                get_app().redraw()
                await curio.sleep(1 / samples)

        self.schedule(runner)


# class OpBuffer:
#     @staticmethod
#     def create(args: Value) -> OperatorRowDescription:
#         pass
#
#     @staticmethod
#     def on_next(self: OperatorSelf, upstream: RowHandle, value: Value) -> Value:
#         pass

@unique
class OperatorIndex(IndexEnum):
    RELAY = auto()
    PROPERTY = auto()
    BUFFER = auto()
    FACTORY = auto()
    COUNTDOWN = auto()
    PRINTER = auto()
    SINE = auto()


ELEMENT_TABLE: Tuple[
    Tuple[
        Optional[Callable[[OperatorSelf, RowHandle], Value]],  # on next
        Optional[Callable[[OperatorSelf, RowHandle], Value]],  # on failure
        Optional[Callable[[OperatorSelf, RowHandle], Value]],  # on completion
        Optional[Callable[[OperatorSelf, RowHandle], None]],  # on subscription
        Callable[[Value], OperatorRowDescription],
    ], ...] = (
    (OpRelay.on_next, None, None, None, OpRelay.create),
    (OpRelay.on_next, None, None, None, OpProperty.create),  # property uses relay's `on_next` function
    (OpBuffer.on_next, None, None, None, OpBuffer.create),
    (OpFactory.on_next, None, None, None, OpFactory.create),
    (OpCountdown.on_next, None, None, None, OpCountdown.create),
    (OpPrinter.on_next, None, None, None, OpPrinter.create),
    (None, None, None, OpSine.on_subscribe, OpSine.create),
)


# SETUP ################################################################################################################

# noinspection PyUnusedLocal
def key_callback(window, key: int, scancode: int, action: int, mods: int) -> None:
    if action not in (glfw.PRESS, glfw.REPEAT):
        return

    if key == glfw.KEY_ESCAPE:
        glfw.set_window_should_close(window, 1)
        return

    if glfw.KEY_A <= key <= glfw.KEY_Z:
        # if (mods & glfw.MOD_SHIFT) != glfw.MOD_SHIFT:
        #     key += 32
        # char: str = bytes([key]).decode()
        get_app().get_scene().get_fact('key_fact').next(Value())


# noinspection PyUnusedLocal
def mouse_button_callback(window, button: int, action: int, mods: int) -> None:
    if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
        x, y = glfw.get_cursor_pos(window)
        get_app().get_scene().get_fact('mouse_fact').next(Value(x, y))


countdown_node: NodeDescription = NodeDescription(
    sockets=dict(
        key_down=(NodeSocketKind.SINK, Value()),
        roundness=(NodeSocketKind.PROPERTY, Value(10)),
    ),
    states=dict(
        default=NodeStateDescription(
            operators=dict(
                factory=(OperatorIndex.FACTORY, Value(id=OperatorIndex.COUNTDOWN, args=Value(start=5))),
                printer=(OperatorIndex.PRINTER, Value(0)),
                mouse_pos_printer=(OperatorIndex.PRINTER, Value(0, 0)),
                sine=(OperatorIndex.SINE, Value()),
            ),
            connections=[
                (Path('/:key_fact'), Path('factory')),
                (Path('factory'), Path('printer')),
                (Path('/:mouse_fact'), Path('mouse_pos_printer')),
                (Path('sine'), Path(':roundness')),
            ],
            design=[
                ("begin_path",),
                ("rounded_rect", 50, 50,
                 Expression('max(0, window.width - 100)'),
                 Expression('max(0, window.height - 100)'),
                 Expression('prop["roundness"]')),
                ("fill_color", nanovg.rgba(.5, .5, .5)),
                ("fill",),
            ],
            children=dict(),
        ),
    ),
    transitions=[],
    initial_state='default',
)

root_node: NodeDescription = NodeDescription(
    sockets=dict(
        key_fact=(NodeSocketKind.SOURCE, Value()),
        mouse_fact=(NodeSocketKind.SOURCE, Value(0, 0)),
    ),
    states=dict(
        default=NodeStateDescription(
            operators={},
            connections=[],
            design=[],
            children=dict(
                herbert=countdown_node,
            ),
        )
    ),
    transitions=[],
    initial_state='default',
)

# count_presses_node: NodeDescription = NodeDescription(
#     sockets=dict(
#         key_down=(NodeSocketKind.SINK, Value())
#     ),
#     states=dict(
#         default=NodeStateDescription(
#             operators=dict(
#                 buffer=(OperatorIndex.BUFFER, Value(schema=Value.Schema(), time_span=1)),
#                 printer=(OperatorIndex.PRINTER, Value()),
#             ),
#             connections=[
#                 (Path('/:key_down'), Path('buffer')),
#                 (Path('buffer'), Path('printer')),
#             ],
#             design=[],
#         ),
#     ),
#     transitions=[],
#     initial_state='default',
# )

# MAIN #################################################################################################################

if __name__ == "__main__":
    sys.exit(get_app().run(root_node))
