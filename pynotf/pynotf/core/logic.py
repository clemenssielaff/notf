from __future__ import annotations

from typing import Callable, Optional, NamedTuple, Tuple, List
from inspect import iscoroutinefunction
from enum import IntEnum, auto, unique
from math import pi, sin
from time import monotonic

import curio
from pyrsistent import field

from pynotf.data import Value, RowHandle, Table, RowHandleList, TableRow, mutate_value
import pynotf.core as core


# DATA #################################################################################################################

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


def create_flags(external: bool = False, multicast: bool = False,
                 status: EmitterStatus = EmitterStatus.IDLE) -> int:
    return (((int(external) << FlagIndex.IS_EXTERNAL) |
             (int(multicast) << FlagIndex.IS_MULTICAST)) &
            ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)


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
        return Operator(core.get_app().get_table(core.TableIndex.OPERATORS).add_row(
            op_index=description.operator_index,
            value=description.initial_value,
            input_schema=description.input_schema,
            args=description.args,
            data=description.data,
            flags=description.flags | (EmitterStatus.IDLE << FlagIndex.STATUS),
        ))

    def __init__(self, handle: RowHandle):
        assert handle.is_null() or handle.table == core.TableIndex.OPERATORS
        self._handle: RowHandle = handle

    def __repr__(self) -> str:
        return f'Operator:{self._handle.index}.{self._handle.generation}'

    def get_handle(self) -> RowHandle:
        return self._handle

    def is_valid(self) -> bool:
        return core.get_app().get_table(core.TableIndex.OPERATORS).is_handle_valid(self._handle)

    def get_op_index(self) -> int:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['op_index']

    def get_flags(self) -> int:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['flags']

    def get_status(self) -> EmitterStatus:
        return EmitterStatus(self.get_flags() >> FlagIndex.STATUS)

    def set_status(self, status: EmitterStatus) -> None:
        table: Table = core.get_app().get_table(core.TableIndex.OPERATORS)
        table[self._handle]['flags'] = \
            (table[self._handle]['flags'] & ~(int(0b111) << FlagIndex.STATUS)) | (status << FlagIndex.STATUS)

    def is_external(self) -> bool:
        return bool(self.get_flags() & (1 << FlagIndex.IS_EXTERNAL))

    def is_multicast(self) -> bool:
        return bool(self.get_flags() & (1 << FlagIndex.IS_MULTICAST))

    def get_input_schema(self) -> Value.Schema:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['input_schema']

    def get_value(self) -> Value:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['value']

    def set_value(self, value: Value, check_schema: Optional[Value.Schema] = None) -> bool:
        if check_schema is not None and check_schema != value.get_schema():
            return False
        core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['value'] = value
        return True

    def get_argument(self, name: str) -> Value:
        """
        :param name: Name of the argument to access.
        :return: The requested argument.
        :raise: KeyError
        """
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]["args"][name]

    def get_data(self) -> Value:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]["data"]

    def get_upstream(self) -> RowHandleList:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['upstream']

    def get_downstream(self) -> RowHandleList:
        return core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['downstream']

    def add_upstream(self, operator: Operator) -> None:
        handle: RowHandle = operator.get_handle()
        current_upstream: RowHandleList = self.get_upstream()
        if handle not in current_upstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['upstream'] = current_upstream.append(
                handle)

    def add_downstream(self, operator: Operator) -> None:
        handle: RowHandle = operator.get_handle()
        current_downstream: RowHandleList = self.get_downstream()
        if handle not in current_downstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['downstream'] = current_downstream.append(
                handle)

    def remove_upstream(self, operator: Operator) -> bool:
        handle: RowHandle = operator.get_handle()
        current_upstream: RowHandleList = self.get_upstream()
        if handle in current_upstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['upstream'] = current_upstream.remove(
                handle)
            return True
        return False

    def remove_downstream(self, operator: Operator) -> bool:
        handle: RowHandle = operator.get_handle()
        current_downstream: RowHandleList = self.get_downstream()
        if handle in current_downstream:
            core.get_app().get_table(core.TableIndex.OPERATORS)[self._handle]['downstream'] = current_downstream.remove(
                handle)
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
        assert isinstance(downstream, Operator)
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
            self._set_data(new_data)

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
        operator_table: Table = core.get_app().get_table(core.TableIndex.OPERATORS)
        status: EmitterStatus = self.get_status()
        if status.is_completed():
            return  # operator has completed

        # mark the operator as actively emitting
        assert not status.is_active()  # cyclic error
        self.set_status(EmitterStatus(callback))  # set the active status

        # copy the list of downstream handles, in case it changes during the emission
        downstream: List[Operator] = [Operator(row_handle) for row_handle in self.get_downstream()]

        if callback == OperatorVtableIndex.NEXT:

            # store the emitted value and ensure that the operator is able to emit the given value
            self.set_value(value, self.get_value().get_schema())

            # emit to all valid downstream elements
            for element in downstream:
                element._run(self, callback, value)

            # reset the status
            self.set_status(EmitterStatus.IDLE)

        else:
            # store the emitted value, bypassing the schema check
            self.set_value(value)

            # emit one last time ...
            for element in downstream:
                element._run(self, callback, value)

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
                self._set_data(result)

        core.get_app().schedule_event(update_data_on_completion)

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
        core.get_app().get_table(core.TableIndex.OPERATORS).remove_row(self._handle)

    def _set_data(self, new_data: Value) -> None:
        operator_table: Table = core.get_app().get_table(core.TableIndex.OPERATORS)
        assert new_data.get_schema() == operator_table[self._handle]["data"].get_schema()
        operator_table[self._handle]['data'] = new_data


# FACT #################################################################################################################

class Fact:
    def __init__(self, operator: Operator):
        self._operator: Operator = operator

    def get_value(self) -> Value:
        return self._operator.get_value()

    def get_schema(self) -> Value.Schema:
        return self.get_value().get_schema()

    def next(self, value: Value) -> None:
        assert value.get_schema() == self.get_schema()
        core.get_app().schedule_event(lambda: self._operator.next(value))

    def fail(self, error: Value) -> None:
        core.get_app().schedule_event(lambda: self._operator.fail(error))

    def complete(self) -> None:
        core.get_app().schedule_event(lambda: self._operator.complete())

    def subscribe(self, downstream: Operator):
        self._operator.subscribe(downstream)


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
                core.get_app().redraw()
                await curio.sleep(1 / samples)

        self.schedule(runner)


@unique
class OperatorIndex(core.IndexEnum):
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
