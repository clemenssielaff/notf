from abc import ABCMeta, abstractmethod
import asyncio
from enum import Enum, auto
from functools import partial
from gc import collect as force_gc
from inspect import iscoroutinefunction
from logging import error as print_error
from threading import Thread, Lock
from traceback import format_exc
from typing import Any, Optional, List, Tuple, Set, Callable
from weakref import ref as weak_ref

from .value import Value


########################################################################################################################

class Circuit:
    pass


class Signal:
    pass


class Emitter:

    def __init__(self, schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param schema:  Schema of the Value emitted by this Emitter, defaults to empty.
        """
        self._downstream: List[weak_ref] = []  # Receivers are ordered but unique
        self._is_completed: bool = False  # Once a Emitter is completed, it can no longer emit any values
        self._output_schema: Value.Schema = schema  # Is constant
        self._error_handler: Callable = self._default_error_handler

    def get_output_schema(self) -> Value.Schema:
        """
        Schema of the emitted value. Can be the empty Schema if this Emitter does not emit a meaningful value.
        """
        return self._output_schema

    def is_completed(self) -> bool:
        """
        Returns true iff the Emitter has been completed, either through an error or normally.
        """
        return self._is_completed

    def set_error_handler(self, error_handler: Callable):
        """
        Installs a new error handler callback on this Emitter.
        Error handlers are functions with the signature:
             handler(receiver: Receiver, exception: Exception) -> None
        :param error_handler:   Error handler to install.
        :raise TypeError:       If the given handler does not match the signature.
        """
        # TODO: check handler signature
        self._error_handler = error_handler

    @staticmethod
    def _default_error_handler(receiver: 'Receiver', exception: Exception):
         """
         Default error handler. Does nothing but dump the exception traceback to stderr.
         All input arguments are ignored.
         :param receiver:   The Receiver that threw the exception.
         :param exception:  The thrown exception.
         """
         print_error(format_exc())

class Receiver:
    def __init__(self, schema: Value.Schema = Value.Schema()):
        """
        Constructor.
        :param schema:  Schema defining the Value expected by this Receiver, defaults to empty.
        """
        self._upstream: List[Emitter] = []  # Emitters are ordered but unique
        self._input_schema: Value.Schema = schema  # Is constant


class Switch(Receiver, Emitter):
    pass

# class Signal:
#         """
#         Whenever a Emitter emits a new Value, it passes along a "Signal" object containing additional information
#         about the source of the Signal and whether or not it can be blocked from being propagated further.
#         The Signal acts as meta-data, which provides essential information for some kind of Switches and the
#         Application Logic.
#
#         The "source" of the Signal is the Emitter that emitted the Value to the Receiver. Since a Receiver is
#         free to connect to any number of different Emitters, and the Value itself does not encode where it was
#         generated, without the source it would be impossible for example to have an Switch that collects values from
#         multiple Emitters in a list each until they complete and then emits a single concatenated Value.
#
#         The "status" of the Signal is relevant for distributing Switches, for example one that propagates a mouse click
#         to all Widgets at that location, in the draw order from top to bottom. The first Widget to accept the click
#         would set the status from "unhandled" to "accepted", notifying later widgets that they should not act unless the
#         programmer chooses to do so regardless. A "blocked" status signifies to the distributing Switch, that it
#         should halt further propagation and return immediately, if for example we want to put a grey overlay over a
#         Widget to signify that this part of the interface has been temporarily disabled without explicitly adding a
#         disabled-state to the lower Widgets.
#         Most Signals are "unblockable", meaning that the Receiver is unable to interfere with their propagation.
#         """
#
#         class Status(Enum):
#             """
#             Status of the Signal with the following state transition diagram:
#                 --> Ignored --> Accepted --> Blocked
#                         |                       ^
#                         +-----------------------+
#                 --> Unblockable
#             """
#             UNBLOCKABLE = 0
#             UNHANDLED = auto()
#             ACCEPTED = auto()
#             BLOCKED = auto()
#
#         def __init__(self, emitter: 'Emitter', is_blockable: bool = False):
#             self._emitter: Emitter = emitter
#             self._status: Emitter.Signal.Status = self.Status.UNHANDLED if is_blockable else self.Status.UNBLOCKABLE
#
#         @property
#         def source(self) -> int:
#             """
#             Numeric ID of the Emitter that emitted the value.
#             """
#             return id(self._emitter)
#
#         def is_blockable(self) -> bool:
#             """
#             Checks if this Signal could be blocked. Note that this also returns true if the Signal has already been
#             blocked and blocking it again would have no effect.
#             """
#             return self._status != self.Status.UNBLOCKABLE
#
#         def is_accepted(self) -> bool:
#             """
#             Returns true iff this Signal is blockable and has been accepted or blocked.
#             """
#             return self._status in (self.Status.ACCEPTED, self.Status.BLOCKED)
#
#         def is_blocked(self) -> bool:
#             """
#             Returns true iff this Signal is blockable and has been blocked.
#             """
#             return self._status == self.Status.BLOCKED
#
#         def accept(self):
#             """
#             If this Signal is blockable and has not been accepted yet, mark it as accepted.
#             """
#             if self._status == self.Status.UNHANDLED:
#                 self._status = self.Status.ACCEPTED
#
#         def block(self):
#             """
#             If this Signal is blockable and has not been blocked yet, mark it as blocked.
#             """
#             if self._status in (self.Status.UNHANDLED, self.Status.ACCEPTED):
#                 self._status = self.Status.BLOCKED


# class Emitter:
#     def emit(self, value: Any = Value()):
#         """
#         Push the given value to all active Receivers.
#         :param value: Value to emit, can be empty if this Emitter does not emit a meaningful value.
#         :raise TypeError: If the Value's Schema doesn't match.
#         :raise LogicError: If the Emitter has already completed (either normally or though an error).
#         """
#         # First, the Emitter needs to check if it is already completed. No point doing anything, if the Emitter is
#         # unable to emit anything. This should never happen, so raise an exception if it does.
#         if self._is_completed:
#             raise LogicError("Cannot emit from a completed Emitter")
#
#         # Check the given value to see if the Emitter is allowed to emit it. If it is not a Value, try to
#         # build one out of it. If that doesn't work, or the Schema of the Value does not match that of the
#         # Emitter, raise an exception.
#         if not isinstance(value, Value):
#             try:
#                 value = Value(value)
#             except ValueError:
#                 raise TypeError("Emitter can only emit values that are implicitly convertible to a Value")
#         if value.schema != self.output_schema:
#             raise TypeError("Emitter cannot emit a value with the wrong Schema")
#
#         # Emit from the local list of strong references that we know will stay alive. The member field may change
#         # during the iteration because some Receiver downstream might add to the list of Receivers or even disconnect
#         # other Receivers, but those changes will not affect the current emitting process.
#         self._emit(self._collect_receivers(), value)
#
#     def _error(self, exception: Exception):
#         """
#         Failure method, completes the Emitter.
#         This method would be protected in C++. It is not private, but should not be part of the public interface.
#         :param exception:   The exception that has occurred.
#         """
#         print_error(format_exc())
#
#         signal: Emitter.Signal = Emitter.Signal(self, is_blockable=False)
#         for receiver in self._collect_receivers():
#             try:
#                 receiver.on_error(signal, exception)
#             except Exception as ex:
#                 print_error(format_exc())
#
#         self._receivers.clear()
#         self._is_completed = True
#
#     def _complete(self):
#         """
#         Completes the Emitter successfully.
#         This method would be protected in C++. It is not private, but should not be part of the public interface.
#         """
#         signal: Emitter.Signal = Emitter.Signal(self, is_blockable=False)
#         for receiver in self._collect_receivers():
#             try:
#                 receiver.on_complete(signal)
#             except Exception as ex:
#                 print_error(format_exc())
#
#         self._receivers.clear()
#         self._is_completed = True
#
#     def _collect_receivers(self) -> List['Receiver']:
#         """
#         Remove all expired Receivers and return a list of strong references to the valid ones.
#         Note that the order of Receivers does not change.
#         """
#         receivers = []
#         valid_index = 0
#         for current_index in range(len(self._receivers)):
#             receiver = self._receivers[current_index]()
#             if receiver is not None:
#                 receivers.append(receiver)
#                 self._receivers[valid_index] = self._receivers[current_index]  # would be a move or swap in C++
#                 valid_index += 1
#         self._receivers = self._receivers[:valid_index]
#         return receivers
#
#     def _emit(self, receivers: List['Receiver'], value: Value):
#         """
#         This method can be overwritten in subclasses to modify the emitting process.
#         At the point where this method is called, we have established that `value` can be emitted by this Emitter,
#         meaning its Schema matches the output Schema of this Emitter and the Emitter has not been completed. All
#         expired Receivers have been removed from the list of Receivers and the `receivers` argument is a list of strong
#         references matching the `self._receivers` weak list.
#         The default implementation of this method simply emits an unblockable Signal to every Receiver in order.
#         Subclasses can do further sorting and if they choose to, can re-apply that ordering to the `_receivers`
#         member field in the hope to speed up sorting the same list in the future using the `_sort_receivers` method.
#         :param receivers: List of Receivers to emit to.
#         :param value: Value to emit.
#         """
#         signal = Emitter.Signal(self, is_blockable=False)
#         for receiver in receivers:
#             try:
#                 receiver.on_next(signal, value)
#             except Exception as exception:
#                 self._handle_exception(receiver, exception)
#
#     def _sort_receivers(self, order: List[int]):
#         """
#         Changes the order of the Receivers of this Emitter without giving write access to the `_receivers` field.
#         Example: Given Receivers [a, b, c], then calling `self._sort_receivers([2, 0, 1])` will change the order
#         to [c, a, b]. Invalid orders will raise an exception.
#         :param order: New order of the receivers.
#         :raise RuntimeError: If the given order is invalid.
#         """
#         if sorted(order) != list(range(len(self._receivers))):
#             raise RuntimeError(f"Invalid order: {order} for a Emitter with {len(self._receivers)} Receivers")
#
#         self._receivers = [self._receivers[index] for index in order]
#
#     def _connect(self, receiver: 'Receiver'):
#         """
#         Adds a new Receiver to receive emitted values.
#         :param receiver: New Receiver.
#         :raise TypeError: If the Receiver's input Schema does not match.
#         """
#         if receiver.input_schema != self.output_schema:
#             raise TypeError("Cannot connect to an Emitter with a different Schema")
#
#         if self._is_completed:
#             receiver.on_complete(Emitter.Signal(self))
#
#         else:
#             weak_receiver = weak_ref(receiver)
#             if weak_receiver not in self._receivers:
#                 self._receivers.append(weak_receiver)
#
#     def _disconnect(self, receiver: 'Receiver'):
#         """
#         Removes the given Receiver if it is connected. If not, the call is ignored.
#         :param receiver: Receiver to disconnect.
#         """
#         weak_receiver = weak_ref(receiver)
#         if weak_receiver in self._receivers:
#             self._receivers.remove(weak_receiver)
#
#     def _handle_exception(self, receiver: 'Receiver', exception: Exception):
#         """
#         This is a virtual method that Emitters can override in order to handle exceptions that occurred in their
#         Receivers `on_next` method.
#         The default implementation simply reports the exception but ultimately ignores it.
#         :param receiver: Receiver that raised the exception.
#         :param exception: Exception raised by receiver.
#         """
#         print_error("Receiver {} failed during Logic evaluation.\nException caught by Emitter {}:\n{}".format(
#             id(receiver), id(self), exception))
#
#
# ########################################################################################################################
#
# class Receiver(metaclass=ABCMeta):
#     """
#     Receivers do not keep track of the Emitters they connect to.
#     An Receiver can be connected to multiple Emitters.
#     """
#
#     def __init__(self, schema: Value.Schema = Value.Schema()):
#         """
#         Constructor.
#         :param schema:  Schema defining the Value expected by this Emitter.
#                         Can be empty if this Receiver does not expect a meaningful value.
#         """
#         self._input_schema: Value.Schema = schema
#         """
#         Is constant.
#         """
#
#     @property
#     def input_schema(self):
#         """
#         The expected value type.
#         """
#         return self._input_schema
#
#     @abstractmethod
#     def on_next(self, signal: Emitter.Signal, value: Optional[Value]):
#         """
#         Abstract method called by any upstream Emitter.
#         :param signal   The Signal associated with this call.
#         :param value    Emitted value, can be None.
#         """
#         pass
#
#     def on_error(self, signal: Emitter.Signal, exception: Exception):
#         """
#         Default implementation of the "error" method: does nothing.
#         :param signal   The Signal associated with this call.
#         :param exception:   The exception that has occurred.
#         """
#         pass
#
#     def on_complete(self, signal: Emitter.Signal):
#         """
#         Default implementation of the "complete" operation, does nothing.
#         :param signal   The Signal associated with this call.
#         """
#         pass
#
#     def connect_to(self, emitter: Emitter):
#         """
#         Connect to the given Emitter.
#         :param emitter:   Emitter to connect to. If this Receiver is already connected, this does nothing.
#         :raise TypeError:   If the Emitters's input Schema does not match.
#         """
#         emitter._connect(self)
#
#     def disconnect_from(self, emitter: Emitter):
#         """
#         Disconnects from the given Emitter.
#         :param emitter:   Emitter to disconnect from. If this Receiver is not connected, this does nothing.
#         """
#         emitter._disconnect(self)
#
#
# ########################################################################################################################
#
# class Switch(Receiver, Emitter):
#     """
#     A Switch is Receiver/Emitter that applies a sequence of Operations to an input value before emitting it
#     (see https://en.wikipedia.org/wiki/Switch).
#     If any Operation throws an exception, the Switch will fail as a whole. Operations are not able to complete the
#     Switch, other than through failure.
#     Not every input is guaranteed to produce an output as Operations are free to store or ignore input values.
#     If an Operation returns a value, it is passed on to the next Operation and if the Operation was the last one in the
#     sequence, its return value is emitted by the Switch.
#     If any Operation does not return a value (returns None), the following Operations are not called and the Switch
#     does not emit anything.
#     """
#
#     class Operation(metaclass=ABCMeta):
#         """
#         Operations are Functors that can be part of a Switch.
#         """
#
#         @property
#         @abstractmethod
#         def input_schema(self) -> Value.Schema:
#             """
#             The Schema of the input value of the Operation.
#             """
#             pass
#
#         @property
#         @abstractmethod
#         def output_schema(self) -> Value.Schema:
#             """
#             The Schema of the output value of the Operation (if there is one).
#             """
#             pass
#
#         @abstractmethod
#         def _perform(self, value: Value) -> Optional[Value]:
#             """
#             Operation implementation.
#             :param value: Input value, conforms to the Schema specified by the `input_schema` property.
#             :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
#                 or None.
#             """
#             pass
#
#         def __call__(self, value: Value) -> Optional[Value]:
#             """
#             Perform the Operation on a given value.
#             :param value: Input value, must conform to the Schema specified by the `input_schema` property.
#             :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
#                 or None.
#             :raise TypeError: If the input Value does not conform to this Operation's input Schema.
#             """
#             if not isinstance(value, Value) or value.schema != self.input_schema:
#                 raise TypeError("Value does not conform to the Operation's input Schema")
#             result: Optional[Value] = self._perform(value)
#             if result is not None:
#                 assert result.schema == self.output_schema
#             return result
#
#     class NoOp(Operation):
#         """
#         An Operation that passes the given Value onwards.
#         Used to create valid but empty Operators.
#         """
#
#         def __init__(self, schema: Value.Schema):
#             """
#             Constructor.
#             :param schema: Schema of the Value to pass through.
#             """
#             self._schema: Value.Schema = schema
#
#         @property
#         def input_schema(self) -> Value.Schema:
#             return self._schema
#
#         @property
#         def output_schema(self) -> Value.Schema:
#             return self._schema
#
#         def _perform(self, value: Value) -> Value:
#             """
#             :param value: Input value.
#             :return: Unchanged input value.
#             """
#             return value
#
#     def __init__(self, *operations: Operation):
#         """
#         Constructor.
#         :param operations: All Operations that this Switch performs in order. Must not be empty.
#         :raise ValueError: If no Operations are passed.
#         :raise TypeError: If two subsequent Operations have mismatched Schemas.
#         """
#         if len(operations) == 0:
#             raise ValueError("Cannot create a Switch without a single Operation")
#
#         for i in range(len(operations) - 1):
#             if operations[i].output_schema != operations[i + 1].input_schema:
#                 raise TypeError(f"Operations {i} and {i + 1} have mismatched Value Schemas")
#
#         self._operations: Tuple[Switch.Operation] = operations
#
#         Receiver.__init__(self, self._operations[0].input_schema)
#         Emitter.__init__(self, self._operations[-1].output_schema)
#
#     def _operate_on(self, value: Value) -> Optional[Value]:
#         """
#         Performs the sequence of Operations on the given Value and returns it or None.
#         Exceptions thrown by a Operation will propagate back to the caller.
#         :param value: Input value.
#         :return: Output of the last Operation or None.
#         """
#         for operation in self._operations:
#             value = operation(value)
#             if value is None:
#                 break
#         return value
#
#     def on_next(self, signal: Emitter.Signal, value: Value):
#         """
#         Performs the sequence of Operations on the given Value and emits the result if one is produced.
#         Exceptions thrown by a Operation will cause the Switch to fail.
#
#         :param signal   The Signal associated with this call.
#         :param value    Input Value conforming to the Switch's input Schema.
#         """
#         try:
#             result = self._operate_on(value)
#         except Exception as exception:
#             self._error(exception)
#         else:
#             if result is not None:
#                 self.emit(result)
#
#
# ########################################################################################################################
#
# class Circuit:
#     """
#     The Application logic runs on a separate thread that is further split up into multiple fibers. In Python, we use the
#     asyncio module for this, in C++ this would be boost.fiber. Regardless, we need a mechanism for cooperative
#     concurrency (not parallelism) that makes sure that only one path of execution is active at any time.
#     It is the executor's job to provide this functionality.
#     """
#
#     class _Status(Enum):
#         """
#         Status of the Executor with the following state transition diagram:
#             --> Running --> Stopping --> Stopped
#                   |                        ^
#                   +------------------------+
#         """
#         RUNNING = 0
#         STOPPING = auto()
#         STOPPED = auto()
#
#     def __init__(self):
#         """
#         Constructor.
#         """
#
#         self._loop: asyncio.AbstractEventLoop = asyncio.new_event_loop()
#         """
#         Python asyncio event loop, used to schedule coroutines.
#         """
#
#         self._thread: Thread = Thread(target=self._run)
#         """
#         Logic thread, runs the Logic's event loop.
#         """
#
#         self._status = self._Status.RUNNING
#         """
#         Used for managing the Executor shutdown process. As soon as the `_stop` coroutine (in Executor.stop) is
#         scheduled, the status switches from RUNNING to either STOPPING or STOPPED. Only an Executor in the RUNNING
#         state is able to schedule new coroutines.
#         The difference between STOPPING and STOPPED is that a STOPPING Executor will wait until all of its scheduled
#         coroutines have finished, whereas a STOPPED Executor will cancel everything that is still waiting and finish
#         the Logic thread as soon as possible.
#         """
#
#         self._mutex: Lock = Lock()
#         """
#         Mutex protecting the status, in C++ an atomic value would do.
#         """
#
#         # start the logic thread immediately
#         self._thread.start()
#
#     def schedule(self, func, *args, **kwargs):
#         """
#         Schedule a new function or coroutine to run if the logic is running.
#
#         :param func: Function to execute in the logic thread.
#         :param args: Variadic arguments for the function.
#         :param kwargs: Keyword arguments for the function.
#         """
#         with self._mutex:
#             if self._status is not self._Status.RUNNING:
#                 return
#         self._schedule(func, *args, **kwargs)
#
#     def _schedule(self, func, *args, **kwargs):
#         """
#         Internal scheduling, is thread-safe.
#
#         :param func: Function to execute in the logic thread.
#         :param args: Variadic arguments for the function.
#         :param kwargs: Keyword arguments for the function.
#         """
#         if iscoroutinefunction(func):
#             self._loop.call_soon_threadsafe(partial(self._loop.create_task, func(*args, **kwargs)))
#         else:
#             self._loop.call_soon_threadsafe(partial(func, *args, **kwargs))
#
#         # always clean up after yourself
#         self._loop.call_soon_threadsafe(self._cleanup)
#
#     def finish(self):
#         """
#         Finish all waiting coroutines and then stop the Executor.
#         """
#         self.stop(force=False)
#
#     def stop(self, force=True):
#         """
#         Stop the Executor, optionally cancelling all waiting coroutines.
#         :param force: If true (the default), all waiting coroutines will be cancelled.
#         """
#         with self._mutex:
#             if self._status is not self._Status.RUNNING:
#                 return
#
#             if force:
#                 self._status = self._Status.STOPPED
#             else:
#                 self._status = self._Status.STOPPING
#
#         async def _stop():
#             self._loop.stop()
#
#         self._schedule(_stop)
#
#         self._thread.join()
#
#     @staticmethod
#     def exception_handler():
#         """
#         Exception handling for exceptions that escape the loop.
#         Prints the exception stack trace.
#         Is a public method so it can be monkey-patched at runtime (e.g. for testing).
#         """
#         print_error(
#             "\n==================================\n"
#             f"{format_exc()}"
#             "==================================\n")
#
#     def _cleanup(self):
#         """
#         Since the order in which the Logic emits values is essentially random, we cannot outright delete Nodes when
#         they are first marked for removal. Instead, we need to remove them after the loop has finished a turn and no
#         change is currently handled.
#         This method does that.
#         """
#         if self._scene.remove_expired_nodes():
#             # We really need those Nodes gone, and their signals, slots and properties with them.
#             # However, many Nodes own Receivers that in turn own references back to the Node, creating
#             # cycles. Those cycles keep the Node and all of its dependents around until the gc comes
#             # around to collect them. We need to do that here.
#             force_gc()
#
#     def _run(self):
#         """
#         Logic thread code.
#         """
#         self._loop.set_exception_handler(lambda *args: self.exception_handler())
#         asyncio.set_event_loop(self._loop)
#
#         try:
#             self._loop.run_forever()
#         finally:
#             pending_tasks = [task for task in asyncio.Task.all_tasks() if not task.done()]
#             with self._mutex:
#                 cancel_all = self._status is self._Status.STOPPED
#                 self._status = self._Status.STOPPED
#             if cancel_all:
#                 for task in pending_tasks:
#                     task.cancel()
#
#             # noinspection PyBroadException
#             try:
#                 self._loop.run_until_complete(asyncio.gather(*pending_tasks))
#             except asyncio.CancelledError:
#                 pass
#             except Exception:
#                 self.exception_handler()
#             finally:
#                 self._loop.close()
