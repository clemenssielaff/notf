from __future__ import annotations

from typing import Callable, Optional, NamedTuple, Tuple, List
from inspect import iscoroutinefunction
from enum import IntEnum, auto, unique
from math import pi, sin
from time import monotonic

import curio
from pyrsistent import field

from pynotf.data import Value, RowHandle, Table, RowHandleList, TableRow, get_mutated_value, Path
import pynotf.core as core

__all__ = ('Operator', 'OperatorRow', 'OperatorIndex', 'OpRelay',
           'OPERATOR_VTABLE', 'OperatorVtableIndex')  # TODO: these should not be part of the public interface


# TODO: Maybe we don't need to store Operators in a table?
#   Only the "state" of the Operator needs to be recoverable, the fact that Operators exist could be implied from the
#   Scene.
#   ... Or not, in the case of dynamically created Operators from the Factory :/ ... Well, in that case, we could store
#   the (Type, Arguments, Data) triplet and still be able to restore them. ... Except not really, because dynamically
#   created Operators would still need their connection information... We also need the flags...
#   Anyway, it would be nice if we could do that, because we could use plain old polymorphism for Operator classes and
#   store actual members, which would avoid storing a lot of unnecessary data in the table.

# DATA #################################################################################################################

@unique
class OperatorEffect(IntEnum):
    """
    Node Interface Operators (Interops) are often used to define how the Node is displayed or laid out by its parent.
    We need some way to redraw or relayout automatically whenever one such Interop changes its value. There were
    multiple possible ways to do it, but we decided to make the "Effect" of an Operator part of its flags.
    The alternatives were:
        1.  Use regular Operators to redraw / relayout the Node
            This has the advantage that it does not need any new concepts. But it either requires all Nodes to have two
            additional "redraw" and "relayout" Operators - or just the "relayout" one, if we move the "redraw" Operator
            into the Scene. However, both options are not great: in the first one, we have two additional Operators for
            each Node - for the second one we still need one additional Operator and one that has a high and constantly
            fluctuating number of upstream connections. We could add a flag indicating that an Operator should not
            keep track of its upstream, but that would add still more complexity.
        2.  Instead of using actual redraw / relayout Operators, use "special" RowHandles as targets.
            This way, we don't actually have to add any new Operators and just need to add a few if-statements whenever
            a Signal is emitted. If the target of the Signal is one of the "special" RowHandles, then we do not emit
            the Signal but call a predefined function instead to redraw or relayout.
            Better, but now we have branching on every single Signal emission.
        3.  Do not use connections at all, but encode the effect of the Interop into the Operator itself.
            This way we only have a single branch on each call to `_emit` and we don't have to store the same "special"
            target Handles in all Interops.
    """
    NONE = 0
    REDRAW = 1
    RELAYOUT = 2


@unique
class OperatorStatus(IntEnum):
    """
    Operators start out IDLE and change to EMITTING whenever they are emitting a ValueSignal to connected Operators.
    If an Operator tries to emit but is already in EMITTING state, we know that we have caught a cyclic dependency.
    To emit a FailureSignal or a CompletionSignal, the Operator will switch to the respective FAILING or COMPLETING
    state. Once it has finished its `_fail` or `_complete` methods, it will permanently change its state to
    COMPLETED or FAILED and will not emit anything again.

        --> IDLE <-> EMITTING
              |
              +--> FAILING --> FAILED
              |
              +--> COMPLETING --> COMPLETE
    """
    EMITTING = 0  # 0-2 matches the corresponding OperatorVtableIndex
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
        return self < OperatorStatus.IDLE

    def is_completed(self) -> bool:
        """
        Every status except IDLE and EMITTING can be considered "completed".
        """
        return not (self == OperatorStatus.IDLE or self == OperatorStatus.EMITTING)


class OperatorFlagIndex(IntEnum):
    """
    Every OperatorRow contains a "Flags" cell (bitmask).
    This enum encodes the location of the various flags within the bitmask.

    IS EXTERNAL
    -----------
    Ownership of free Operators is shared by their downstream. Operators that are attached to Nodes or Services on the
    other hand are owned by some external entity and persist even without subscribers.
    0:  this Operator is owned by its downstream, when the last one disconnects the Operator will be deleted
    1:  this Operator is owned externally, meaning it is destroyed explicitly at some point

    IS SOURCE
    ------------
    Some Operators, like Facts, only emit values and never receive any.
    This flag is checked in `Operator.subscribe` to make sure that you cannot connect to a source Operator downstream.
    0:  this Operator accepts connections from upstream
    1:  this Operator does not allow upstream connections

    EFFECT
    ------
    See `OperatorEffect` enum.

    STATUS
    ------
    See `OperatorStatus` enum.
    """
    IS_EXTERNAL = 0  # size: 1
    IS_SOURCE = 1  # size: 1
    EFFECT = 2  # size: 2
    STATUS = 4  # size: 3


class OperatorRow(TableRow):
    """
    I had a long long about splitting this table up into several tables, as not all Operators require access to all
    fields. The Relay, for example, only requires a Schema, a status and the downstream. And many other operators might
    not require the data field.
    However, I have finally decided to use a single table (for now, and maybe ever). The reason is that our goal is to
    keep memory local. If we have a table with wide rows, the jumps between each row is large. However, if you were to
    jump between tables, the distances would be much larger. You'd save memory overall, but the access pattern is worse.
    Also, having everything in a single table means we don't have to have special cases for different tables (emission
    from a table with a list to store the downstream VS. emission from a table that only has a single downstream entry
    etc.).

    If the Operator table is too wide (the data in a single row is large enough so you have a lot of cache misses when
    you jump around in the table), we could store a Box<T> as value instead. This would keep the table itself small (a
    lot smaller than it currently is), but you'd have a jump to random memory whenever you access a row, which might be
    worse...
    """
    __table_index__: int = core.TableIndex.OPERATORS
    op_index = field(type=int, mandatory=True)
    flags = field(type=int, mandatory=True)  # flags and op_index could be stored in the same 64 bit word
    node = field(type=RowHandle, mandatory=True)
    value = field(type=Value, mandatory=True)
    input_schema = field(type=Value.Schema, mandatory=True)
    args = field(type=Value, mandatory=True)  # must be a named record
    data = field(type=Value, mandatory=True)  # must be a named record
    name = field(type=str, mandatory=True, initial='')  # this is just for debugging purposes
    expression = field(type=core.Expression, mandatory=True, initial=core.Expression())
    upstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())
    downstream = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# OPERATORS ############################################################################################################

class Operator:
    Effect = OperatorEffect

    class Description(NamedTuple):
        """
        All Operator `create` methods return an Operator.Description instead of an actual Operator instance.
        This allows us to inspect potential Operators without actually instantiating them.
        The Factory Operator, for example, needs to know the input/output Schema of an Operator-to-be.
        Operator.Descriptions are turned into actual Operator instances by calling `Operator.create`.
        """
        operator_index: int  # Entry in the `OperatorIndex` table
        initial_value: Value  # Initial value of the Operator
        input_schema: Value.Schema = Value.Schema()  # Schema of the Operator's input
        args: Value = Value()
        data: Value = Value()
        expression: str = ''
        flags: int = 0

        @staticmethod
        def create_flags(external: bool = False, source: bool = False) -> int:
            """
            Creates a value for the "flags" bitmask of the Operator.Description.
            """
            return (int(external) << OperatorFlagIndex.IS_EXTERNAL) | (int(source) << OperatorFlagIndex.IS_SOURCE)

    @classmethod
    def create(cls, description: Description, node: RowHandle = RowHandle(), effect: Effect = Effect.NONE,
               name: str = "") -> Operator:
        """
        Takes an Operator.Description and (optional) per-instance arguments and creates an Operator instance.
        :param description: Date from which to construct the new row.
        :param node: (optional) Node to associate with the created Operator.
        :param effect: (optional) Effect of the Operator, only applies if `node` is set.
        :param name: (optional) Name of the Operator on the Node, only for debugging purposes.
        :return: The created Operator.
        """
        return Operator(core.get_app().get_table(core.TableIndex.OPERATORS).add_row(
            op_index=description.operator_index,
            flags=(description.flags
                   | ((effect if node else OperatorEffect.NONE) << OperatorFlagIndex.EFFECT)
                   | (OperatorStatus.IDLE << OperatorFlagIndex.STATUS)),
            node=node,
            value=description.initial_value,
            input_schema=description.input_schema,
            args=description.args,
            data=description.data,
            expression=core.Expression(description.expression),
            name=name,
        ))

    def __init__(self, handle: RowHandle):
        assert handle.is_null() or handle.table == core.TableIndex.OPERATORS
        self._handle: RowHandle = handle

    def __repr__(self) -> str:
        self_row: Table.Accessor = core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]

        node: core.Node = core.Node(self_row['node'])
        node_path: str = ''
        if node.is_valid():
            node_path = str(node.get_path())

        given_name: str = self_row['name']
        if node_path and given_name:
            return f'Operator "{node_path}{Path.INTEROP_DELIMITER}{given_name}"'
        elif given_name:
            return f'Operator "{given_name}"'
        else:
            return f'Operator:{self._handle.index}.{self._handle.generation}'

    def get_handle(self) -> RowHandle:
        return self._handle

    def is_valid(self) -> bool:
        return core.get_app().get_table(core.TableIndex.OPERATORS).is_handle_valid(self._handle)

    def get_input_schema(self) -> Value.Schema:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['input_schema']

    def get_value(self) -> Value:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['value']

    def get_argument(self, name: str) -> Value:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]["args"][name]

    def get_data(self) -> Value:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]["data"]

    def get_expression(self) -> core.Expression:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]["expression"]

    def _set_data(self, new_data: Value) -> None:
        operator_table: Table = core.get_app().get_table(core.TableIndex.OPERATORS)
        assert new_data.get_schema() == operator_table[self._handle]["data"].get_schema()
        operator_table[self._handle]['data'] = new_data

    def _get_op_index(self) -> int:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['op_index']

    def _get_node(self) -> core.Node:
        return core.Node(core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['node'])

    def _get_flags(self) -> int:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['flags']

    def _get_status(self) -> OperatorStatus:
        return OperatorStatus(self._get_flags() >> OperatorFlagIndex.STATUS)

    def _set_status(self, status: OperatorStatus) -> None:
        table: Table = core.get_app().get_table(core.TableIndex.OPERATORS)
        table[self._handle]['flags'] = (
                (table[self._handle]['flags'] & ~(int(0b111) << OperatorFlagIndex.STATUS))
                | (status << OperatorFlagIndex.STATUS))

    def _is_external(self) -> bool:
        # TODO: is the `is_external` flag maybe superfluous if we can test for a Node instead?
        return bool(self._get_flags() & (1 << OperatorFlagIndex.IS_EXTERNAL)) or self._get_node().is_valid()

    def _is_source(self) -> bool:
        return bool(self._get_flags() & (1 << OperatorFlagIndex.IS_SOURCE))

    def _get_effect(self) -> OperatorEffect:
        return OperatorEffect((self._get_flags() >> OperatorFlagIndex.EFFECT) & 0b11)

    def _get_upstream(self) -> RowHandleList:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['upstream']

    def get_downstream(self) -> RowHandleList:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['downstream']

    def _add_upstream(self, operator: Operator) -> None:
        handle: RowHandle = operator.get_handle()
        current_upstream: RowHandleList = self._get_upstream()
        if handle not in current_upstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['upstream'] = current_upstream.append(
                handle)

    def _add_downstream(self, operator: Operator) -> None:
        handle: RowHandle = operator.get_handle()
        current_downstream: RowHandleList = self.get_downstream()
        if handle not in current_downstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['downstream'] = current_downstream.append(
                handle)

    def _remove_upstream(self, operator: Operator) -> bool:
        handle: RowHandle = operator.get_handle()
        current_upstream: RowHandleList = self._get_upstream()
        if handle in current_upstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['upstream'] = current_upstream.remove(
                handle)
            return True
        return False

    def _remove_downstream(self, operator: Operator) -> bool:
        handle: RowHandle = operator.get_handle()
        current_downstream: RowHandleList = self.get_downstream()
        if handle in current_downstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['downstream'] = current_downstream.remove(
                handle)
            return True
        return False

    def subscribe(self, downstream: Operator) -> None:
        if downstream._is_source():
            raise RuntimeError(f"Cannot subscribe Source Operator {str(downstream)}")

        if not downstream.get_input_schema().is_any() and not (
                self.get_value().get_schema() == downstream.get_input_schema()):
            raise RuntimeError("Schema mismatch")

        # if the operator has already completed, call the corresponding callback on the downstream operator
        # immediately and do not subscribe it
        my_status: OperatorStatus = self._get_status()
        if my_status.is_completed():
            assert self._is_external()  # operator is valid but completed? -> it must be external
            callback = OperatorVtableIndex(my_status if my_status.is_active() else int(my_status) - 3)
            downstream._run(self, callback, self.get_value())
            return

        self._add_downstream(downstream)
        downstream._add_upstream(self)

        # execute the operator's `on_subscribe` callback
        on_subscribe_func: Optional[Callable] = OPERATOR_VTABLE[self._get_op_index()][OperatorVtableIndex.SUBSCRIPTION]
        if on_subscribe_func is not None:
            on_subscribe_func(self, downstream)

    def _unsubscribe(self, downstream: Operator) -> None:
        assert isinstance(downstream, Operator)
        # TODO: I don't really have a good idea yet how to handle error and completion values.
        #  they are also stored in the `value` field, but that screws with the schema compatibility check
        # assert get_input_schema(downstream).is_none() \
        #   or (get_value(upstream).get_schema() == get_input_schema(downstream))

        # if the upstream was already completed when the downstream subscribed, its subscription won't have completed
        current_upstream: RowHandleList = downstream._get_upstream()
        if self._handle not in current_upstream:
            assert self._get_status().is_completed()
            assert len(self.get_downstream()) == 0
            return

        # update the downstream element
        downstream._remove_upstream(self)

        # update the upstream element
        self._remove_downstream(downstream)

        # if the upstream element was internal and this was its last subscriber, remove it
        if len(self.get_downstream()) == 0 and not self._is_external():
            return self.remove()

    def on_update(self, source: Operator, value: Value) -> None:
        self._run(source, OperatorVtableIndex.UPDATE, value)

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
        status: OperatorStatus = self._get_status()
        if status.is_completed():
            return  # operator has completed

        # find the callback to perform
        callback_func: Optional[Callable] = OPERATOR_VTABLE[self._get_op_index()][callback]
        if callback_func is None:
            return  # operator type does not provide the requested callback

        if callback == OperatorVtableIndex.UPDATE:
            # perform the `on_update` callback
            new_data: Value = callback_func(self, source, value)

            # ...and update the operator's data
            self._set_data(new_data)

        else:
            # the failure and completion callbacks do not return a value
            callback_func(self, source, value)

    def update(self, *args) -> None:
        value: Value
        if len(args) == 0:
            raise ValueError("Called `Operator.update` without an argument")
        if len(args) == 1 and isinstance(args[0], Value):
            value = args[0]
        else:
            value = Value(*args)
        self._emit(OperatorVtableIndex.UPDATE, value)

    def fail(self, error: Value) -> None:
        self._emit(OperatorVtableIndex.FAILURE, error)

    def complete(self, message: Optional[Value] = None) -> None:
        self._emit(OperatorVtableIndex.COMPLETION, message or Value())

    def _emit(self, callback: OperatorVtableIndex, value: Value) -> None:
        assert callback != OperatorVtableIndex.CREATE

        # make sure the operator is valid and not completed yet
        operator_table: Table = core.get_app().get_table(core.TableIndex.OPERATORS)
        status: OperatorStatus = self._get_status()
        if status.is_completed():
            return  # operator has completed

        # mark the operator as actively emitting
        assert not status.is_active()  # cyclic error
        self._set_status(OperatorStatus(callback))  # set the active status

        # copy the list of downstream handles, in case it changes during the emission
        downstream: List[Operator] = [Operator(row_handle) for row_handle in self.get_downstream()]

        if callback == OperatorVtableIndex.UPDATE:
            # ensure that the operator is able to emit the given value
            # callbacks other than UPDATE are allowed to store non-schema conform data (
            if self.get_value().get_schema() != value.get_schema():
                raise ValueError(f'Cannot emit Value with incompatible Schema from {self!r}\n'
                                 f'Expected\n{self.get_value().get_schema()}got\n{value.get_schema()}')

        # store the emitted value
        core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['value'] = value

        # emit to all valid downstream elements
        for element in downstream:
            element._run(self, callback, value)

        if callback == OperatorVtableIndex.UPDATE:
            # reset the status
            self._set_status(OperatorStatus.IDLE)

            # perform additional effects
            if self._get_effect() == OperatorEffect.REDRAW:
                core.get_app().redraw()  # TODO: selective redraw
            elif self._get_effect() == OperatorEffect.RELAYOUT:
                pass  # TODO: relayout effect

        else:
            # finalize the status
            self._set_status(OperatorStatus(int(callback) + 3))

            # unsubscribe all downstream handles (this might destroy the Operator, therefore it is the last thing we do)
            for element in downstream:
                self._unsubscribe(element)

            # if the Operator is external, it will still be alive after everyone unsubscribed
            if operator_table.is_handle_valid(self._handle):
                assert self._is_external()
                assert len(self.get_downstream()) == 0

    def schedule(self, callback: Callable, *args):
        assert iscoroutinefunction(callback)

        async def update_data_on_completion():
            result: Optional[Value] = await callback(*args)

            # only update the operator data if it has not completed
            if self.is_valid():
                self._set_data(result)

        core.get_app().schedule_event(update_data_on_completion)

    def remove(self) -> None:
        if not self.is_valid():
            return

        # remove from all remaining downstream elements
        for downstream in self.get_downstream():
            Operator(downstream)._remove_upstream(self)

        # unsubscribe from all upstream elements
        for upstream in self._get_upstream():
            Operator(upstream)._unsubscribe(self)  # this might remove upstream elements

        # finally, remove yourself
        core.get_app().get_table(core.TableIndex.OPERATORS).remove_row(self._handle)


# OPERATOR REGISTRY ####################################################################################################

class OpRelay:
    @staticmethod
    def create(value: Value) -> Operator.Description:
        return Operator.Description(
            operator_index=OperatorIndex.RELAY,
            initial_value=value,
            input_schema=value.get_schema(),
        )

    @staticmethod
    def on_update(self: Operator, _: RowHandle, value: Value) -> Value:
        self.update(value)
        return self.get_data()


class OpBuffer:
    @staticmethod
    def create(args: Value) -> Operator.Description:
        schema: Value.Schema = Value.Schema.from_value(args['schema'])
        return Operator.Description(
            operator_index=OperatorIndex.BUFFER,
            initial_value=Value(schema),
            input_schema=schema,
            args=Value(time_span=args['time_span']),
            data=Value(is_running=False, counter=0),
        )

    @staticmethod
    def on_update(self: Operator, _upstream: RowHandle, _value: Value) -> Value:
        if not int(self.get_data()['is_running']) == 1:
            async def timeout():
                await curio.sleep(float(self.get_argument("time_span")))
                if not self.is_valid():
                    return
                self.update(Value(self.get_data()['counter']))
                print(f'Clicked {int(self.get_data()["counter"])} times '
                      f'in the last {float(self.get_argument("time_span"))} seconds')
                return get_mutated_value(self.get_data(), "is_running", False)

            self.schedule(timeout)
            return Value(is_running=True, counter=1)

        else:
            return get_mutated_value(self.get_data(), "counter", self.get_data()['counter'] + 1)


class OpFactory:
    """
    Creates an Operator that takes an empty Signal and produces and subscribes a new Operator for each subscription.
    """

    @staticmethod
    def create(args: Value) -> Operator.Description:
        operator_id: int = int(args['id'])
        factory_arguments: Value = args['args']
        example_operator_data: Operator.Description = OPERATOR_VTABLE[operator_id][OperatorVtableIndex.CREATE](
            factory_arguments)

        return Operator.Description(
            operator_index=OperatorIndex.FACTORY,
            initial_value=example_operator_data.initial_value,
            args=args,
        )

    @staticmethod
    def on_update(self: Operator, _1: RowHandle, _2: Value) -> Value:
        downstream: RowHandleList = self.get_downstream()
        if len(downstream) == 0:
            return self.get_data()

        factory_function: Callable[[Value], Operator.Description] = OPERATOR_VTABLE[int(self.get_argument('id'))][
            OperatorVtableIndex.CREATE]
        factory_arguments: Value = self.get_argument('args')
        for subscriber in downstream:
            new_operator: Operator = Operator.create(factory_function(factory_arguments))
            new_operator.subscribe(Operator(subscriber))
            new_operator.on_update(self, new_operator.get_value())

        return self.get_data()


class OpCountdown:
    @staticmethod
    def create(args: Value) -> Operator.Description:
        return Operator.Description(
            operator_index=OperatorIndex.COUNTDOWN,
            initial_value=Value(0),
            args=args,
        )

    @staticmethod
    def on_update(self: Operator, _upstream: RowHandle, _value: Value) -> Value:
        counter: Value = self.get_argument('start')
        assert counter.get_kind() == Value.Kind.NUMBER

        async def loop():
            nonlocal counter
            self.update(counter)
            while counter > 0:
                counter = counter - 1
                await curio.sleep(1)
                self.update(counter)
            self.complete()

        self.schedule(loop)
        return self.get_data()


class OpPrinter:
    @staticmethod
    def create(value: Value) -> Operator.Description:
        return Operator.Description(
            operator_index=OperatorIndex.PRINTER,
            initial_value=value,
            input_schema=value.get_schema(),
        )

    @staticmethod
    def on_update(self: Operator, upstream: RowHandle, value: Value) -> Value:
        print(f'Received {value!r} from {upstream}')
        return self.get_data()


class OpSine:
    @staticmethod
    def create(args: Value) -> Operator.Description:
        frequency: float = 0.2
        amplitude: float = 100
        samples: float = 72  # samples per second (this results in about 60 fps)
        keys: List[str] = args.get_keys()
        if 'frequency' in keys:
            frequency = float(args['frequency'])
        if 'amplitude' in keys:
            amplitude = float(args['amplitude'])
        if 'samples' in keys:
            amplitude = float(args['samples'])

        return Operator.Description(
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
                self.update(Value((sin(2 * pi * frequency * monotonic()) + 1) * amplitude * 0.5))
                await curio.sleep(1 / samples)

        self.schedule(runner)


class OpNodeExpression:
    """
    This Operator executes an arbitrary expression on the Node that it is attached to.
    I am not 100% sure that this is a good idea yet, but it seems like a very powerful and general approach.
    The specific use-case that brought this about is that I want to update the built-in xform interop of a node with
    just a position. We could do that outside of the node as well, but I would like to try this approach and see where
    it takes me.
    ... Actually, it seems like this was quite intended by myself long ago when Interops were called Properties and they
    had Callbacks instead of connections into state-defined operators. I actually like the state-defined operator
    approach better.

    Arguments:
        source -- Expression source with the signature: (node: Node, arg: Value) -> None
        schema -- Expression argument Schema.
    """

    @staticmethod
    def create(args: Value) -> Operator.Description:
        return Operator.Description(
            operator_index=OperatorIndex.NODE_EXPRESSION,
            initial_value=Value(),
            input_schema=Value.Schema.from_value(args['schema']),
            expression=str(args['source']),
        )

    @staticmethod
    def on_update(self: Operator, _: RowHandle, value: Value) -> Value:
        self.get_expression().execute(
            node=self._get_node(),  # TODO: NodeAccessor
            arg=value
        )

        return self.get_data()


class OpFilterExpression:
    """
    Expression Operator that takes an input Value (of arbitrary but fixed Schema) and emits the same Value, but only if
    it passes the filter function.

    Arguments:
        source -- Expression string with the signature: (node: Node, arg: Value) -> bool
        schema -- Expression argument Schema.
    """

    @staticmethod
    def create(args: Value) -> Operator.Description:
        # the operator needs to know the schema of the value it is operating on
        value_schema: Optional[Value.Schema] = Value.Schema.from_value(args['schema'])

        return Operator.Description(
            operator_index=OperatorIndex.FILTER_EXPRESSION,
            initial_value=Value(value_schema),  # zero initialized
            input_schema=value_schema,
            expression=str(args['source']),
        )

    @staticmethod
    def on_update(self: Operator, _: RowHandle, value: Value) -> Value:
        pass_filter: bool = self.get_expression().execute(
            node=core.ConstNodeAccessor(self._get_node()),
            arg=value
        )
        if pass_filter:
            self.update(value)

        return self.get_data()


class OpNodeTransition:
    """
    Transitions the Node from one state to another.

    Arguments:
        target -- Name of the target state.
    """

    @staticmethod
    def create(args: Value) -> Operator.Description:
        return Operator.Description(
            operator_index=OperatorIndex.NODE_TRANSITION,
            initial_value=Value(),
            input_schema=Value.Schema.any(),
            args=Value(
                target=str(args['target']),
            )
        )

    @staticmethod
    def on_update(self: Operator, _1: RowHandle, _2: Value) -> Value:
        self._get_node().transition_into(str(self.get_argument('target')))
        return self.get_data()


@unique
class OperatorIndex(core.IndexEnum):
    RELAY = auto()
    BUFFER = auto()
    FACTORY = auto()
    COUNTDOWN = auto()
    PRINTER = auto()
    SINE = auto()
    NODE_EXPRESSION = auto()
    FILTER_EXPRESSION = auto()
    NODE_TRANSITION = auto()


@unique
class OperatorVtableIndex(IntEnum):
    UPDATE = 0  # 0-2 matches the corresponding EmitterStatus
    FAILURE = 1
    COMPLETION = 2
    SUBSCRIPTION = 3
    CREATE = 4


OPERATOR_VTABLE: Tuple[
    Tuple[
        Optional[Callable[[Operator, RowHandle], Value]],  # on update
        Optional[Callable[[Operator, RowHandle], Value]],  # on failure
        Optional[Callable[[Operator, RowHandle], Value]],  # on completion
        Optional[Callable[[Operator, RowHandle], None]],  # on subscription
        Callable[[Value], Operator.Description],  # factory
    ], ...] = (
    (OpRelay.on_update, None, None, None, OpRelay.create),
    (OpBuffer.on_update, None, None, None, OpBuffer.create),
    (OpFactory.on_update, None, None, None, OpFactory.create),
    (OpCountdown.on_update, None, None, None, OpCountdown.create),
    (OpPrinter.on_update, None, None, None, OpPrinter.create),
    (None, None, None, OpSine.on_subscribe, OpSine.create),
    (OpNodeExpression.on_update, None, None, None, OpNodeExpression.create),
    (OpFilterExpression.on_update, None, None, None, OpFilterExpression.create),
    (OpNodeTransition.on_update, None, None, None, OpNodeTransition.create),
)
