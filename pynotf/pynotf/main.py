from __future__ import annotations

import sys
from enum import IntEnum, auto, unique
from logging import warning
from threading import Thread
from typing import Tuple, Callable, Optional, Dict, Union, List, NamedTuple, Iterable

from inspect import iscoroutinefunction
from math import sin, pi
from time import monotonic

import glfw
import curio

from pyrsistent._checked_types import CheckedPMap as ConstMap
from pyrsistent import field

import pynotf.extra.pynanovg as nanovg
from pycnotf import V2f, Size2f, Aabrf

from pynotf.data import Value, RowHandle, Table, TableRow, Storage, mutate_value, RowHandleList, Bonsai, RowHandleMap, \
    Path, Claim, Shape
from pynotf.core.event_loop import EventLoop
from pynotf.core.data_types import Xform, EmitterStatus, ValueList, IndexEnum
from pynotf.core.painterpreter import Sketch, Painter, Design
from pynotf.extra.svg import parse_svg


# DATA #################################################################################################################

@unique
class TableIndex(IndexEnum):
    OPERATORS = auto()
    NODES = auto()
    LAYOUTS = auto()


class FlagIndex(IntEnum):
    IS_EXTERNAL = 0  # if this Operator is owned externally, meaning it is destroyed explicitly at some point
    IS_MULTICAST = 1  # if this Operator allows more than one downstream subscriber
    STATUS = 2  # offset of the EmitterStatus


def create_flags(external: bool = False, multicast: bool = False, status: EmitterStatus = EmitterStatus.IDLE) -> int:
    return (((int(external) << FlagIndex.IS_EXTERNAL) |
             (int(multicast) << FlagIndex.IS_MULTICAST)) &
            ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)


class LayoutDescription(NamedTuple):
    type: LayoutIndex
    args: Value


# TODO: Node states carry a lot of weight right now and I think they are a crutch.
#  Instead, you should be able to to create child and dynamic operators and change the layout and claim etc. without
#  changing the state.
class NodeStateDescription(NamedTuple):
    operators: Dict[str, Tuple[OperatorIndex, Value]]
    connections: List[Tuple[Path, Path]]
    design: Design
    children: Dict[str, NodeDescription]
    layout: LayoutDescription
    claim: Claim = Claim()


class NodeProperties(NamedTuple):
    names: Bonsai
    elements: List[Operator]


class NodeDescription(NamedTuple):
    properties: Dict[str, Value]
    states: Dict[str, NodeStateDescription]
    transitions: List[Tuple[str, str]]
    initial_state: str


BUILTIN_NAMESPACE = 'sys'

BUILTIN_NODE_PROPERTIES: Dict[str, Value] = {
    f'{BUILTIN_NAMESPACE}.opacity': Value(1),
    f'{BUILTIN_NAMESPACE}.visibility': Value(1),
    f'{BUILTIN_NAMESPACE}.depth': Value(0),
    f'{BUILTIN_NAMESPACE}.xform': Value(1, 0, 0, 1, 0, 0),
    # f'{BUILTIN_NAMESPACE}.claim' : Value(0), # TODO: express Claim as wrapper around Value
    f'{BUILTIN_NAMESPACE}.height': Value(100),  # and remove the {BUILTIN_NAMESPACE}.height/width props
    f'{BUILTIN_NAMESPACE}.width': Value(100),  # and remove the {BUILTIN_NAMESPACE}.height/width props
}


class Hitbox(NamedTuple):
    aabr: Aabrf
    callback: Operator


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
    args = field(type=Value, mandatory=True)  # must be a named record
    data = field(type=Value, mandatory=True)  # must be a named record
    upstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


class OperatorRowDescription(NamedTuple):
    operator_index: int
    initial_value: Value
    input_schema: Value.Schema = Value.Schema()
    args: Value = Value()
    data: Value = Value()
    flags: int = create_flags()


class NodeComposition(NamedTuple):
    xform: Xform  # layout xform
    grant: Size2f  # grant size
    opacity: float = 0


class NodeCompositions(ConstMap):
    __key_type__ = str
    __value_type__ = NodeComposition


class LayoutComposition(NamedTuple):
    nodes: NodeCompositions = NodeCompositions()  # node name -> composition
    order: RowHandleList = RowHandleList()  # all nodes in draw order
    aabr: Aabrf = Aabrf()  # union of all child aabrs
    claim: Claim = Claim()  # union of all child claims


class LayoutRow(TableRow):
    __table_index__: int = TableIndex.LAYOUTS
    layout_index = field(type=int, mandatory=True)
    args = field(type=Value, mandatory=True)  # must be a named record
    node_value_initial = field(type=Value, mandatory=True)
    nodes = field(type=RowHandleList, mandatory=True, initial=RowHandleList())
    node_values = field(type=ValueList, mandatory=True, initial=ValueList())
    composition = field(type=LayoutComposition, mandatory=True, initial=LayoutComposition())


class NodeRow(TableRow):
    __table_index__: int = TableIndex.NODES
    description = field(type=NodeDescription, mandatory=True)
    parent = field(type=RowHandle, mandatory=True)
    properties = field(type=NodeProperties, mandatory=True)
    layout = field(type=RowHandle, mandatory=True, initial=RowHandle())
    state = field(type=str, mandatory=True, initial='')
    child_names = field(type=RowHandleMap, mandatory=True, initial=RowHandleMap())
    network = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# FUNCTIONS ############################################################################################################

def is_in_aabr(aabr: Aabrf, pos: V2f) -> bool:
    return aabr.min.x <= pos.x <= aabr.max.x and aabr.min.y <= pos.y <= aabr.max.y


def patch_builtin_properties(node_description: NodeDescription) -> None:
    # ensure none of the properties use the the reserved 'sys' namespace
    for property_name in node_description.properties:
        assert not property_name.startswith(f'{BUILTIN_NAMESPACE}.')

    # add built-in properties
    node_description.properties.update(BUILTIN_NODE_PROPERTIES)


# OPERATORS ############################################################################################################


class Operator:

    @classmethod
    def create(cls, description: OperatorRowDescription) -> Operator:
        """
        For example, for the Factory operator, we want to inspect the input/output Schema of another, yet-to-be-created
        Operator without actually creating one.
        Therefore, the creator functions only return OperatorRowDescription, that *this* function then turns into an
        actual row in the Operator table.
        :param description: Date from which to construct the new row.
        :return: The handle to the created row.
        """
        return Operator(get_app().get_table(TableIndex.OPERATORS).add_row(
            op_index=description.operator_index,
            value=description.initial_value,
            input_schema=description.input_schema,
            args=description.args,
            data=description.data,
            flags=description.flags | (EmitterStatus.IDLE << FlagIndex.STATUS),
        ))

    def __init__(self, handle: RowHandle):
        assert handle.is_null() or handle.table == TableIndex.OPERATORS
        self._handle: RowHandle = handle

    def __repr__(self) -> str:
        return f'Operator:{self._handle.index}.{self._handle.generation}'

    def get_handle(self) -> RowHandle:
        return self._handle

    def is_valid(self) -> bool:
        return get_app().get_table(TableIndex.OPERATORS).is_handle_valid(self._handle)

    def get_op_index(self) -> int:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]['op_index']

    def get_flags(self) -> int:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]['flags']

    def get_status(self) -> EmitterStatus:
        return EmitterStatus(self.get_flags() >> FlagIndex.STATUS)

    def set_status(self, status: EmitterStatus) -> None:
        table: Table = get_app().get_table(TableIndex.OPERATORS)
        table[self._handle]['flags'] = \
            (table[self._handle]['flags'] & ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)

    def is_external(self) -> bool:
        return bool(self.get_flags() & (1 << FlagIndex.IS_EXTERNAL))

    def is_multicast(self) -> bool:
        return bool(self.get_flags() & (1 << FlagIndex.IS_MULTICAST))

    def get_input_schema(self) -> Value.Schema:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]['input_schema']

    def get_value(self) -> Value:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]['value']

    def set_value(self, value: Value, check_schema: Optional[Value.Schema] = None) -> bool:
        if check_schema is not None and check_schema != value.get_schema():
            return False
        get_app().get_table(TableIndex.OPERATORS)[self._handle]['value'] = value
        return True

    def get_argument(self, name: str) -> Value:
        """
        :param name: Name of the argument to access.
        :return: The requested argument.
        :raise: KeyError
        """
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["args"][name]

    def get_data(self) -> Value:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]["data"]

    def get_upstream(self) -> RowHandleList:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]['upstream']

    def get_downstream(self) -> RowHandleList:
        return get_app().get_table(TableIndex.OPERATORS)[self._handle]['downstream']

    def add_upstream(self, operator: Operator) -> None:
        handle: RowHandle = operator.get_handle()
        current_upstream: RowHandleList = self.get_upstream()
        if handle not in current_upstream:
            get_app().get_table(TableIndex.OPERATORS)[self._handle]['upstream'] = current_upstream.append(handle)

    def add_downstream(self, operator: Operator) -> None:
        handle: RowHandle = operator.get_handle()
        current_downstream: RowHandleList = self.get_downstream()
        if handle not in current_downstream:
            get_app().get_table(TableIndex.OPERATORS)[self._handle]['downstream'] = current_downstream.append(handle)

    def remove_upstream(self, operator: Operator) -> bool:
        handle: RowHandle = operator.get_handle()
        current_upstream: RowHandleList = self.get_upstream()
        if handle in current_upstream:
            get_app().get_table(TableIndex.OPERATORS)[self._handle]['upstream'] = current_upstream.remove(handle)
            return True
        return False

    def remove_downstream(self, operator: Operator) -> bool:
        handle: RowHandle = operator.get_handle()
        current_downstream: RowHandleList = self.get_downstream()
        if handle in current_downstream:
            get_app().get_table(TableIndex.OPERATORS)[self._handle]['downstream'] = current_downstream.remove(handle)
            return True
        return False

    def subscribe(self, downstream: Operator) -> None:
        assert downstream.get_input_schema().is_none() or (
                self.get_value().get_schema() == downstream.get_input_schema())

        # if the operator has already completed, call the corresponding callback on the downstream operator
        # immediately and do not subscribe it
        my_status: EmitterStatus = self.get_status()
        if my_status.is_completed():
            assert self.is_external()  # operator is valid but completed? -> it must be external
            callback = OperatorVtableIndex(my_status if my_status.is_active() else int(my_status) - 3)
            downstream._run(self, callback, self.get_value())
            return

        if len(self.get_downstream()) != 0:
            assert self.is_multicast()

        self.add_downstream(downstream)
        downstream.add_upstream(self)

        # execute the operator's `on_subscribe` callback
        on_subscribe_func: Optional[Callable] = OPERATOR_VTABLE[self.get_op_index()][OperatorVtableIndex.SUBSCRIPTION]
        if on_subscribe_func is not None:
            on_subscribe_func(self, downstream)

    def unsubscribe(self, downstream: Operator) -> None:
        # TODO: I don't really have a good idea yet how to handle error and completion values.
        #  they are also stored in the `value` field, but that screws with the schema compatibility check
        # assert get_input_schema(downstream).is_none() or (get_value(upstream).get_schema() == get_input_schema(downstream))

        # if the upstream was already completed when the downstream subscribed, its subscription won't have completed
        current_upstream: RowHandleList = downstream.get_upstream()
        if self._handle not in current_upstream:
            assert self.get_status().is_completed()
            assert len(self.get_downstream()) == 0
            return

        # update the downstream element
        downstream.remove_upstream(self)

        # update the upstream element
        self.remove_downstream(downstream)

        # if the upstream element was internal and this was its last subscriber, remove it
        if len(self.get_downstream()) == 0 and not self.is_external():
            return self.remove()

    def on_next(self, source: Operator, value: Value) -> None:
        self._run(source, OperatorVtableIndex.NEXT, value)

    def on_fail(self, source: Operator, error: Value) -> None:
        self._run(source, OperatorVtableIndex.FAILURE, error)

    def on_complete(self, source: Operator, message: Optional[Value] = None) -> None:
        self._run(source, OperatorVtableIndex.COMPLETION, message or Value())

    def _run(self, source: Operator, callback: OperatorVtableIndex, value: Value) -> None:
        """
        Runs one of the three callbacks of this Operator.
        """
        assert source.is_valid()

        # make sure the operator is valid and not completed yet
        operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
        status: EmitterStatus = self.get_status()
        if status.is_completed():
            return  # operator has completed

        # find the callback to perform
        callback_func: Optional[Callable] = OPERATOR_VTABLE[self.get_op_index()][callback]
        if callback_func is None:
            return  # operator type does not provide the requested callback

        if callback == OperatorVtableIndex.NEXT:
            # perform the `on_next` callback
            new_data: Value = callback_func(self, source, Value() if self.get_input_schema().is_none() else value)

            # ...and update the operator's data
            assert new_data.get_schema() == operator_table[self._handle]["data"].get_schema()
            operator_table[self._handle]['data'] = new_data

        else:
            # the failure and completion callbacks do not return a value
            callback_func(self, source, value)

    def next(self, value: Value) -> None:
        self._emit(OperatorVtableIndex.NEXT, value)

    def fail(self, error: Value) -> None:
        self._emit(OperatorVtableIndex.FAILURE, error)

    def complete(self, message: Optional[Value] = None) -> None:
        self._emit(OperatorVtableIndex.COMPLETION, message or Value())

    def _emit(self, callback: OperatorVtableIndex, value: Value) -> None:
        assert callback != OperatorVtableIndex.CREATE

        # make sure the operator is valid and not completed yet
        operator_table: Table = get_app().get_table(TableIndex.OPERATORS)
        status: EmitterStatus = self.get_status()
        if status.is_completed():
            return  # operator has completed

        # mark the operator as actively emitting
        assert not status.is_active()  # cyclic error
        self.set_status(EmitterStatus(callback))  # set the active status

        # copy the list of downstream handles, in case it changes during the emission
        downstream: RowHandleList = self.get_downstream()

        if callback == OperatorVtableIndex.NEXT:

            # store the emitted value and ensure that the operator is able to emit the given value
            self.set_value(value, self.get_value().get_schema())

            # emit to all valid downstream elements
            for element in downstream:
                Operator(element)._run(self, callback, value)

            # reset the status
            self.set_status(EmitterStatus.IDLE)

        else:
            # store the emitted value, bypassing the schema check
            self.set_value(value)

            # emit one last time ...
            for element in downstream:
                Operator(element)._run(self, callback, value)

            # ... and finalize the status
            self.set_status(EmitterStatus(int(callback) + 3))

            # unsubscribe all downstream handles (this might destroy the Operator, therefore it is the last thing we do)
            for element in downstream:
                self.unsubscribe(element)

            # if the Operator is external, it will still be alive after everyone unsubscribed
            if operator_table.is_handle_valid(self._handle):
                assert self.is_external()
                assert len(self.get_downstream()) == 0

    def schedule(self, callback: Callable, *args):
        assert iscoroutinefunction(callback)

        async def update_data_on_completion():
            result: Optional[Value] = await callback(*args)

            # only update the operator data if it has not completed
            if self.is_valid():
                self.set_value(result)

        get_app().schedule_event(update_data_on_completion)

    def remove(self) -> None:
        if not self.is_valid():
            return

        # remove from all remaining downstream elements
        for downstream in self.get_downstream():
            Operator(downstream).remove_upstream(self)

        # unsubscribe from all upstream elements
        for upstream in self.get_upstream():
            Operator(upstream).unsubscribe(self)  # this might remove upstream elements

        # finally, remove yourself
        get_app().get_table(TableIndex.OPERATORS).remove_row(self._handle)


# APPLICATION ##########################################################################################################

class Application:

    def __init__(self):
        self._storage: Storage = Storage(
            operators=OperatorRow,
            nodes=NodeRow,
            layouts=LayoutRow,
        )
        self._event_loop: EventLoop = EventLoop()
        self._scene: Scene = Scene()

    def get_table(self, table_index: Union[int, TableIndex]) -> Table:
        return self._storage[int(table_index)]

    def is_handle_valid(self, handle: RowHandle) -> bool:
        if handle.is_null():
            return False
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


# LOGIC ################################################################################################################

class Fact:
    def __init__(self, operator: Operator):
        self._operator: Operator = operator

    def get_value(self) -> Value:
        return self._operator.get_value()

    def get_schema(self) -> Value.Schema:
        return self.get_value().get_schema()

    def next(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        get_app().schedule_event(lambda: self._operator.next(value))

    def fail(self, error: Value) -> None:
        get_app().schedule_event(lambda: self._operator.fail(error))

    def complete(self) -> None:
        get_app().schedule_event(lambda: self._operator.complete())

    def subscribe(self, downstream: Operator):
        self._operator.subscribe(downstream)


# SCENE ################################################################################################################

class Scene:

    def __init__(self):
        self._root_node: Optional[Node] = None
        self._hitboxes: List[Hitbox] = []
        self._size: Size2f = Size2f()

    def initialize(self, root_description: NodeDescription):
        # create the root node
        patch_builtin_properties(root_description)
        self._root_node = Node(get_app().get_table(TableIndex.NODES).add_row(
            description=root_description,
            parent=RowHandle(),  # empty row handle as parent
            properties=NodeProperties(
                names=Bonsai([property_name for property_name in root_description.properties.keys()]),
                elements=[Operator.create(OpRelay.create(value)) for value in root_description.properties.values()],
            )
        ))
        self._root_node.transition_into(root_description.initial_state)

    def set_size(self, size: Size2f):
        if size != self._size:
            self._size = size
            self._root_node.relayout_down(self._size)

    def get_size(self) -> Size2f:
        return self._size

    def clear(self):
        if self._root_node:
            self._root_node.remove()

    def create_node(self, name: str, description: NodeDescription) -> Node:
        return self._root_node.create_child(name, description)

    def get_fact(self, name: str, ) -> Optional[Fact]:
        fact_handle: Optional[Operator] = self._root_node.get_property(name)
        if fact_handle:
            return Fact(fact_handle)
        else:
            return None

    def get_node(self, path: Path) -> Optional[Node]:
        return self._root_node.get_descendant(path)

    def paint(self, painter: Painter) -> None:
        self._root_node.paint(painter)
        self._hitboxes = painter.get_hitboxes()

    def iter_hitboxes(self, pos: V2f) -> Iterable[Hitbox]:
        """
        Returns the Operators connected to all hitboxes that overlap the given position in reverse draw order.
        """
        for hitbox in self._hitboxes:
            if is_in_aabr(hitbox.aabr, pos):
                yield hitbox


class Node:
    def __init__(self, handle: RowHandle):
        assert handle.is_null() or handle.table == TableIndex.NODES
        self._handle: RowHandle = handle

    def get_handle(self) -> RowHandle:
        return self._handle

    def get_parent(self) -> Optional[Node]:
        parent_handle: RowHandle = get_app().get_table(TableIndex.NODES)[self._handle]['parent']
        if not parent_handle:
            return None
        return Node(parent_handle)

    def get_layout(self) -> Layout:
        return Layout(get_app().get_table(TableIndex.NODES)[self._handle]['layout'])

    def get_composition(self) -> NodeComposition:
        parent: Optional[Node] = self.get_parent()
        if not parent:
            return NodeComposition(Xform(), get_app().get_scene().get_size(), 1)  # root composition

        node_composition: Optional[Optional[NodeComposition]] = parent.get_layout().get_node_composition(self)
        assert node_composition
        return node_composition

    def create_child(self, name: str, description: NodeDescription) -> Node:
        # ensure the child name is unique
        node_table: Table = get_app().get_table(TableIndex.NODES)
        assert name not in node_table[self._handle]['child_names']

        # create the child node entry
        patch_builtin_properties(description)
        child_handle: RowHandle = node_table.add_row(
            description=description,
            parent=self._handle,
            properties=NodeProperties(
                names=Bonsai([property_name for property_name in description.properties.keys()]),
                elements=[Operator.create(OpRelay.create(Value(property_schema))) for
                          property_schema in description.properties.values()],
            )
        )
        node_table[self._handle]['child_names'] = node_table[self._handle]['child_names'].set(name, child_handle)

        # initialize the child node by transitioning into its initial_state state
        child_node: Node = Node(child_handle)
        child_node.transition_into(description.initial_state)

        return child_node

    # TODO: it might be faster to just store the name inside the node as well..
    def get_name(self) -> str:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if parent_handle.is_null():
            return '/'
        child_names: RowHandleMap = node_table[parent_handle]['child_names']
        for index, (child_name, child_handle) in enumerate(child_names.items()):
            if child_handle == self._handle:
                return child_name
        assert False

    def get_child(self, name: str) -> Optional[Node]:
        child_handle: Optional[RowHandle] = get_app().get_table(TableIndex.NODES)[self._handle]['child_names'].get(name)
        if child_handle is None:
            return None
        return Node(child_handle)

    def get_descendant(self, path: Path, step: int = 0) -> Optional[Node]:
        # success
        if step == len(path):
            return self

        # next step is up
        if path[step] == Path.STEP_UP:
            parent: Optional[Node] = self.get_parent()
            if parent:
                return parent.get_descendant(path, step + 1)

        # next step is down
        else:
            next_child: Optional[Node] = self.get_child(path[step])
            if next_child:
                return next_child.get_descendant(path, step + 1)

        # failure
        return None

    def paint(self, painter: Painter):
        painter.paint(self)
        for node_handle in self.get_layout().get_composition().order:
            painter.paint(Node(node_handle))

    def get_property(self, name: str) -> Optional[Operator]:
        properties: NodeProperties = get_app().get_table(TableIndex.NODES)[self._handle]['properties']
        index: Optional[int] = properties.names.get(name)
        if index is None:
            return None
        return properties.elements[index]

    def get_state(self) -> str:
        return get_app().get_table(TableIndex.NODES)[self._handle]['state']

    def get_sketch(self) -> Sketch:
        node_table: Table = get_app().get_table(TableIndex.NODES)
        design: Design = node_table[self._handle]['description'].states[self.get_state()].design
        return design.sketch(self)

    def remove(self):
        # unregister from the parent
        node_table: Table = get_app().get_table(TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if parent_handle:
            child_names: RowHandleMap = node_table[parent_handle]['child_names']
            node_table[parent_handle]['child_names'] = child_names.remove(self.get_name())

        # remove all children first and then yourself
        self._remove_recursively()

    def transition_into(self, state: str) -> None:
        # ensure the transition is allowed
        node_table: Table = get_app().get_table(TableIndex.NODES)
        current_state: str = node_table[self._handle]['state']
        node_description: NodeDescription = node_table[self._handle]['description']
        assert current_state == '' or (current_state, state) in node_description.transitions

        # remove the dynamic dependencies from the last state
        # TODO: remove children, or think of a better way to transition between states
        self._clear_dependencies()

        # enter the new state
        node_table[self._handle]['state'] = state
        new_state: NodeStateDescription = node_description.states[state]

        # create new child nodes
        child_nodes: List[Node] = []
        for name, child_description in new_state.children.items():
            child_nodes.append(self.create_child(name, child_description))

        # create the new layout
        layout: Layout = Layout.create(new_state.layout.type, new_state.layout.args)
        for child_node in child_nodes:
            layout.add_node(child_node)
        node_table[self._handle]['layout'] = layout.get_handle()

        # create new operators
        network: Dict[str, RowHandle] = {}
        for name, (kind, args) in new_state.operators.items():
            assert name not in network
            network[name] = Operator.create(OPERATOR_VTABLE[kind][OperatorVtableIndex.CREATE](args)).get_handle()
        node_table[self._handle]['network'] = RowHandleList(network.values())

        def find_operator(path: Path) -> Optional[Operator]:
            if path.is_node_path():
                assert path.is_relative() and len(path) == 1  # interpret single name paths as dynamic operator
                return Operator(network.get(path[0]))

            else:
                assert path.is_leaf_path()
                node: Optional[Node]
                if path.is_absolute():
                    node = get_app().get_scene().get_node(path)
                else:
                    assert path.is_relative()
                    node = self.get_descendant(path)
                if node:
                    return node.get_property(path.get_leaf())

            return None

        # create new connections
        for source, sink in new_state.connections:
            find_operator(source).subscribe(find_operator(sink))

        # update the claim
        self.set_claim(new_state.claim)  # causes a potential re-layout

    def _remove_recursively(self):
        """
        Unlike `remove`, this does not remove this Node from its parent as we know that the parent is also in the
        process of removing itself from the Scene.
        """
        # remove all children
        node_table: Table = get_app().get_table(TableIndex.NODES)
        for child_handle in node_table[self._handle]['child_names'].values():
            Node(child_handle)._remove_recursively()

        # remove dynamic network
        self._clear_dependencies()

        # remove properties
        properties: NodeProperties = node_table[self._handle]['properties']
        for property_ in properties.elements:
            property_.remove()

        # remove the node
        node_table.remove_row(self._handle)

    def _clear_dependencies(self) -> None:
        node_table: Table = get_app().get_table(TableIndex.NODES)

        # network
        for operator in node_table[self._handle]['network']:
            Operator(operator).remove()
        node_table[self._handle]['network'] = RowHandleList()

        # layout
        self.get_layout().remove()

    def relayout_down(self, grant: Size2f) -> None:
        # update the layout with the new grant
        layout: Layout = self.get_layout()
        old_layout: LayoutComposition = layout.get_composition()
        new_layout: LayoutComposition = layout.perform(grant)

        # update all child nodes
        for child_name, child_layout in new_layout.nodes.items():
            assert isinstance(child_name, str)
            assert isinstance(child_layout, NodeComposition)
            old_node_layout: Optional[NodeComposition] = old_layout.nodes.get(child_name)
            if old_node_layout and old_node_layout.grant == child_layout.grant:
                continue  # no change in grant
            child_node: Optional[Node] = self.get_child(child_name)
            if not child_node:
                warning(f'Layout contained unknown child node "{child_name}"')
                continue
            child_node.relayout_down(grant)

    def set_claim(self, claim: Claim) -> None:
        # TODO: relayout upwards
        if not self.get_parent():
            return
        self.get_property('sys.width').set_value(Value(claim.horizontal.preferred))
        self.get_property('sys.height').set_value(Value(claim.vertical.preferred))


# LAYOUT REGISTRY ######################################################################################################

class Layout:

    def __init__(self, handle: RowHandle):
        assert handle.is_null() or handle.table == TableIndex.LAYOUTS
        self._handle: RowHandle = handle

    @staticmethod
    def create(layout_index: LayoutIndex, args: Value) -> Layout:
        return Layout(LAYOUT_VTABLE[layout_index][LayoutVtableIndex.CREATE](args))

    def get_handle(self) -> RowHandle:
        return self._handle

    def is_valid(self) -> bool:
        return get_app().get_table(TableIndex.LAYOUTS).is_handle_valid(self._handle)

    def get_argument(self, name) -> Value:
        """
        :param name: Name of the argument to access.
        :return: The requested argument.
        :raise: KeyError
        """
        return get_app().get_table(TableIndex.LAYOUTS)[self._handle]["args"][name]

    def clear(self) -> None:
        get_app().get_table(TableIndex.LAYOUTS)[self._handle]['nodes'] = RowHandleList()

    def add_node(self, node: Node) -> None:
        layout_table: Table = get_app().get_table(TableIndex.LAYOUTS)
        current_nodes: RowHandleList = layout_table[self._handle]['nodes']
        if node in current_nodes:
            current_nodes = current_nodes.remove(node)  # re-add at the end
        layout_table[self._handle]['nodes'] = current_nodes.append(node.get_handle())

    def get_composition(self) -> LayoutComposition:
        return get_app().get_table(TableIndex.LAYOUTS)[self._handle]['composition']

    def get_node_composition(self, node: Node) -> Optional[NodeComposition]:
        return self.get_composition().nodes.get(node.get_name())

    def perform(self, grant: Size2f) -> LayoutComposition:
        layout_table: Table = get_app().get_table(TableIndex.LAYOUTS)
        layout_index: LayoutIndex = layout_table[self._handle]['layout_index']
        composition: LayoutComposition = LAYOUT_VTABLE[layout_index][LayoutVtableIndex.LAYOUT](self, grant)
        layout_table[self._handle]['composition'] = composition
        return composition

    def get_nodes(self) -> Iterable[LayoutNodeView]:
        """
        Returns a Node View for each Node in this layout, in the order that they were added.
        """
        node_list: RowHandleList = get_app().get_table(TableIndex.LAYOUTS)[self._handle]['nodes']
        return (LayoutNodeView(Node(handle)) for handle in node_list)

    def get_order(self) -> RowHandleList:
        """
        Returns the handles of all Nodes in this Layout in draw-order.
        """
        node_list: RowHandleList = get_app().get_table(TableIndex.LAYOUTS)[self._handle]['nodes']
        return RowHandleList(sorted(node_list, key=lambda h: float(Node(h).get_property('sys.depth').get_value())))

    def remove(self) -> None:
        if self.is_valid():
            get_app().get_table(TableIndex.LAYOUTS).remove_row(self._handle)


class LayoutNodeView:
    def __init__(self, node: Node):
        self._node: Node = node

    @property
    def name(self) -> str:
        return self._node.get_name()

    @property
    def opacity(self) -> float:
        return float(self._node.get_property('sys.opacity').get_value())

    @property
    def claim(self) -> Claim:
        width = float(self._node.get_property('sys.width').get_value())
        height = float(self._node.get_property('sys.height').get_value())
        return Claim(Claim.Stretch(width, max_=200), Claim.Stretch(height, max_=200))  # TODO: hack


class LtOverlayout:
    @staticmethod
    def create(args: Value) -> RowHandle:
        return get_app().get_table(TableIndex.LAYOUTS).add_row(
            layout_index=LayoutIndex.OVERLAY,
            args=args,
            node_value_initial=Value(),
        )

    @staticmethod
    def layout(self: Layout, grant: Size2f) -> LayoutComposition:
        compositions: Dict[str, NodeComposition] = {}
        for node_view in self.get_nodes():
            node_grant: Size2f = Size2f(
                width=max(node_view.claim.horizontal.min, min(node_view.claim.horizontal.max, grant.width)),
                height=max(node_view.claim.vertical.min, min(node_view.claim.vertical.max, grant.height)),
            )
            assert node_view.name not in compositions
            compositions[node_view.name] = NodeComposition(
                xform=Xform(),
                grant=node_grant,
                opacity=node_view.opacity,
            )
        return LayoutComposition(
            nodes=NodeCompositions(compositions),
            order=self.get_order(),
            aabr=Aabrf(grant),
            claim=Claim(),
        )


class LtFlexbox:
    @staticmethod
    def create(args: Value) -> RowHandle:
        return get_app().get_table(TableIndex.LAYOUTS).add_row(
            layout_index=LayoutIndex.FLEXBOX,
            args=args,
            node_value_initial=Value(),
        )

    @staticmethod
    def layout(self: Layout, grant: Size2f) -> LayoutComposition:
        compositions: Dict[str, NodeComposition] = {}
        x_offset: float = 0
        for node_view in self.get_nodes():
            node_grant: Size2f = Size2f(
                width=max(node_view.claim.horizontal.min, min(node_view.claim.horizontal.max, grant.width)),
                height=max(node_view.claim.vertical.min, min(node_view.claim.vertical.max, grant.height)),
            )
            assert node_view.name not in compositions
            compositions[node_view.name] = NodeComposition(
                xform=Xform(e=x_offset),
                grant=node_grant,
                opacity=node_view.opacity,
            )
            x_offset += node_grant.width + float(self.get_argument('margin'))

        return LayoutComposition(
            nodes=NodeCompositions(compositions),
            order=self.get_order(),
            aabr=Aabrf(grant),
            claim=Claim(),
        )


@unique
class LayoutIndex(IndexEnum):
    OVERLAY = auto()
    FLEXBOX = auto()


@unique
class LayoutVtableIndex(IntEnum):
    CREATE = 0
    LAYOUT = 1


LAYOUT_VTABLE: Tuple[
    Tuple[
        Optional[Callable[[Value], RowHandle]],  # create
        Optional[Callable[[Layout, Size2f], LayoutComposition]],  # layout
    ], ...] = (
    (LtOverlayout.create, LtOverlayout.layout),
    (LtFlexbox.create, LtFlexbox.layout),
)


# OPERATOR REGISTRY ####################################################################################################

class OpRelay:
    @staticmethod
    def create(value: Value) -> OperatorRowDescription:
        return OperatorRowDescription(
            operator_index=OperatorIndex.RELAY,
            initial_value=value,
            input_schema=value.get_schema(),
            flags=(1 << FlagIndex.IS_MULTICAST),
        )

    @staticmethod
    def on_next(self: Operator, _: RowHandle, value: Value) -> Value:
        self.next(value)
        return self.get_data()


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
    def on_next(self: Operator, _upstream: RowHandle, _value: Value) -> Value:
        if not int(self.get_data()['is_running']) == 1:
            async def timeout():
                await curio.sleep(float(self.get_argument("time_span")))
                if not self.is_valid():
                    return
                self.next(Value(self.get_data()['counter']))
                print(f'Clicked {int(self.get_data()["counter"])} times '
                      f'in the last {float(self.get_argument("time_span"))} seconds')
                return mutate_value(self.get_data(), False, "is_running")

            self.schedule(timeout)
            return Value(is_running=True, counter=1)

        else:
            return mutate_value(self.get_data(), self.get_data()['counter'] + 1, "counter")


class OpFactory:
    """
    Creates an Operator that takes an empty Signal and produces and subscribes a new Operator for each subscription.
    """

    @staticmethod
    def create(args: Value) -> OperatorRowDescription:
        operator_id: int = int(args['id'])
        factory_arguments: Value = args['args']
        example_operator_data: OperatorRowDescription = OPERATOR_VTABLE[operator_id][OperatorVtableIndex.CREATE](
            factory_arguments)

        return OperatorRowDescription(
            operator_index=OperatorIndex.FACTORY,
            initial_value=example_operator_data.initial_value,
            args=args,
        )

    @staticmethod
    def on_next(self: Operator, _1: RowHandle, _2: Value) -> Value:
        downstream: RowHandleList = self.get_downstream()
        if len(downstream) == 0:
            return self.get_data()

        factory_function: Callable[[Value], OperatorRowDescription] = OPERATOR_VTABLE[int(self.get_argument('id'))][
            OperatorVtableIndex.CREATE]
        factory_arguments: Value = self.get_argument('args')
        for subscriber in downstream:
            new_operator: Operator = Operator.create(factory_function(factory_arguments))
            new_operator.subscribe(Operator(subscriber))
            new_operator.on_next(self, new_operator.get_value())

        return self.get_data()


class OpCountdown:
    @staticmethod
    def create(args: Value) -> OperatorRowDescription:
        return OperatorRowDescription(
            operator_index=OperatorIndex.COUNTDOWN,
            initial_value=Value(0),
            args=args,
        )

    @staticmethod
    def on_next(self: Operator, _upstream: RowHandle, _value: Value) -> Value:
        counter: Value = self.get_argument('start')
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
        return self.get_data()


class OpPrinter:
    @staticmethod
    def create(value: Value) -> OperatorRowDescription:
        return OperatorRowDescription(
            operator_index=OperatorIndex.PRINTER,
            initial_value=value,
            input_schema=value.get_schema(),
        )

    @staticmethod
    def on_next(self: Operator, upstream: RowHandle, value: Value) -> Value:
        print(f'Received {value!r} from {upstream}')
        return self.get_data()


class OpSine:
    @staticmethod
    def create(args: Value) -> OperatorRowDescription:
        frequency: float = 0.5
        amplitude: float = 100
        samples: float = 72  # samples per second (this results in about 60 fps)
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
    def on_subscribe(self: Operator, _1: RowHandle) -> None:
        frequency: float = float(self.get_argument('frequency'))
        amplitude: float = float(self.get_argument('amplitude'))
        samples: float = float(self.get_argument('samples'))

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
    SINE = auto()


@unique
class OperatorVtableIndex(IntEnum):
    NEXT = 0  # 0-2 matches the corresponding EmitterStatus
    FAILURE = 1
    COMPLETION = 2
    SUBSCRIPTION = 3
    CREATE = 4


OPERATOR_VTABLE: Tuple[
    Tuple[
        Optional[Callable[[Operator, RowHandle], Value]],  # on next
        Optional[Callable[[Operator, RowHandle], Value]],  # on failure
        Optional[Callable[[Operator, RowHandle], Value]],  # on completion
        Optional[Callable[[Operator, RowHandle], None]],  # on subscription
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
        get_app().get_scene().get_fact('key_fact').next(Value())


# noinspection PyUnusedLocal
def mouse_button_callback(window, button: int, action: int, mods: int) -> None:
    if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
        x, y = glfw.get_cursor_pos(window)
        get_app().get_scene().get_fact('mouse_fact').next(Value(x, y))
    elif button == glfw.MOUSE_BUTTON_RIGHT and action == glfw.PRESS:
        x, y = glfw.get_cursor_pos(window)
        for hitbox in get_app().get_scene().iter_hitboxes(V2f(x, y)):
            aabr = hitbox.aabr
            x1, y1, x2, y2 = aabr.min.x, aabr.min.y, aabr.max.x, aabr.max.y
            print(f"Triggered hitbox callback for hitbox with aabr({x1}, {y1}, {x2}, {y2})")
        # get_app().get_scene().get_fact('mouse_fact').next(Value(x, y))


# noinspection PyUnusedLocal
def window_size_callback(window, width: int, height: int) -> None:
    get_app().get_scene().set_size(Size2f(width, height))


NOTF_SHAPES: List[Shape] = []
for path_description in parse_svg('/home/clemens/code/notf/dev/img/notf_inline_white.svg'):
    NOTF_SHAPES.extend(path_description.shapes)

count_presses_node: NodeDescription = NodeDescription(
    properties=dict(
        key_down=Value(),
        bring_front=Value(),
    ),
    states=dict(
        default=NodeStateDescription(
            operators=dict(
                buffer=(OperatorIndex.BUFFER, Value(schema=Value.Schema(), time_span=1)),
                printer=(OperatorIndex.PRINTER, Value()),
            ),
            connections=[
                (Path('/:mouse_fact'), Path('buffer')),
                (Path('buffer'), Path('printer')),
            ],
            design=Design(
                # Design.FillCall(shape=Design.RoundedRect(
                #     x=Design.Constant(Value(0)), y=Design.Constant(Value(0)),
                #     width=Design.Expression(Value(0).get_schema(), "max(0, node.grant.width)"),
                #     height=Design.Expression(Value(0).get_schema(), "max(0, node.grant.height)"),
                #     radius=Design.Constant(Value(0))),
                #     paint=Design.SolidColor(Design.Constant(Value(r=1, g=1, b=1, a=1))),
                Design.FillCall(
                    shape=Design.ConstantShapes(NOTF_SHAPES),
                    paint=Design.SolidColor(Design.Constant(Value(r=1, g=1, b=1, a=1))),
                )),
            children=dict(),
            layout=LayoutDescription(
                LayoutIndex.OVERLAY,
                Value(),
            ),
            claim=Claim(Claim.Stretch(10, 200, 200), Claim.Stretch(10, 200, 200)),
        ),
    ),
    transitions=[],
    initial_state='default',
)

countdown_node: NodeDescription = NodeDescription(
    properties=dict(
        key_down=Value(),
        roundness=Value(10),
        bring_front=Value(),
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
            design=Design(
                Design.FillCall(shape=Design.RoundedRect(
                    x=Design.Constant(Value(0)), y=Design.Constant(Value(0)),
                    width=Design.Expression(Value(0).get_schema(), "max(0, node.grant.width)"),
                    height=Design.Expression(Value(0).get_schema(), "max(0, node.grant.height)"),
                    radius=Design.Expression(Value(0).get_schema(), "max(0, node.roundness)")),
                    paint=Design.SolidColor(Design.Constant(Value(r=.5, g=.5, b=.5, a=1))),
                )),
            children=dict(),
            layout=LayoutDescription(
                LayoutIndex.OVERLAY,
                Value(),
            ),
            claim=Claim(Claim.Stretch(100, 200, 800), Claim.Stretch(100, 200, 800)),
        ),
    ),
    transitions=[],
    initial_state='default',
)

root_node: NodeDescription = NodeDescription(
    properties=dict(
        key_fact=Value(),
        mouse_fact=Value(0, 0),
        hitbox_fact=Value(0, 0),
    ),
    states=dict(
        default=NodeStateDescription(
            operators={},
            connections=[],
            design=Design(),
            children=dict(
                # herbert=countdown_node,
                zanzibar=count_presses_node,
            ),
            layout=LayoutDescription(
                LayoutIndex.FLEXBOX,
                Value(margin=-100),
            ),
            claim=Claim(),
        )
    ),
    transitions=[],
    initial_state='default',
)

# MAIN #################################################################################################################

if __name__ == "__main__":
    sys.exit(get_app().run(root_node))
