from __future__ import annotations
from typing import Optional, Set, Union
from weakref import ref as weak_ref

from ..value import Value

from .circuit import Circuit
from .signals import ValueSignal, FailureSignal, CompletionSignal


########################################################################################################################
# noinspection PyAbstractClass
class Receiver(Circuit.Element):
    """
    Virtual interface class for Circuit Elements that receive Signals.
    Is non-copyable and may only be owned by other Receivers downstream and Nodes.
    """

    def __init__(self, schema: Value.Schema = Value.Schema()):  # noexcept
        """
        Constructor.
        Should only be called by `Circuit.create_receiver` and the Operator constructor.
        :param schema:  Schema defining the Value expected by this Receiver, defaults to empty.
        """
        self._input_schema: Value.Schema = schema  # is constant
        self._upstream: Set[Emitter] = set()

    def get_input_schema(self) -> Value.Schema:  # noexcept
        """
        Schema of expected values. Can be the empty Schema.
        """
        return self._input_schema

    def has_upstream(self) -> bool:  # noexcept
        """
        Checks if the Receiver has any Emitter connected upstream.
        Useful for overrides of `_on_failure` and `_on_complete` to find out whether the completed Emitter was the last
        connected one.
        """
        return len(self._upstream) != 0

    def create_operator(self, operation: 'Operator.Operation') -> Optional['Operator.CreatorHandle']:  # noexcept
        """
        Creates and connects to a new Operator upstream.
        :param operation: Operation defining the Operator.
        :returns: New Operator or None if the output type of the Operation does not match this Receiver's.
        """
        # fail immediately if the schemas don't match
        if self.get_input_schema() != operation.get_output_schema():
            return self.get_circuit().handle_error(Circuit.Error(
                weak_ref(self),
                Circuit.Error.Kind.WRONG_VALUE_SCHEMA,
                f"Cannot create an Operator with the output Value Schema {operation.get_output_schema()} "
                f"from a Receiver with the input Value Schema {self.get_input_schema()}"))

        # create the Operator
        operator: Operator = self.get_circuit().create_element(Operator, operation)

        # store the operator right away to keep it alive, this does not count as a connection yet
        self._upstream.add(operator)

        # the actual connection is delayed until the end of the event
        self.get_circuit().create_connection(operator, self)

        # return a handle to the freshly created Operator
        return Operator.CreatorHandle(operator)

    def connect_to(self, emitter: Union[Emitter, Emitter.Handle]):  # noexcept
        """
        Connect to the given Emitter.
        If the Emitter is invalid or this Receiver is already connected to it, this does nothing.
        :param emitter: Emitter / Handle of the Emitter to connect to.
        """
        # get the emitter to connect to, if the input is a valid handle
        if isinstance(emitter, Emitter.Handle):
            emitter: Optional[Emitter] = emitter._element()
            if emitter is None:
                return

        # fail immediately if the schemas don't match
        if self.get_input_schema() != emitter.get_output_schema():
            return self.get_circuit().handle_error(Circuit.Error(
                weak_ref(self),
                Circuit.Error.Kind.WRONG_VALUE_SCHEMA,
                f"Cannot connect Emitter {emitter.get_id()} "
                f"with the output Value Schema {emitter.get_output_schema()} "
                f"to a Receiver with the input Value Schema {self.get_input_schema()}"))

        # store the operator right away to keep it alive
        # as the emitter does not know about this, it does not count as a connection yet
        self._upstream.add(emitter)

        # schedule the creation of the connection
        self.get_circuit().create_connection(emitter, self)

    def disconnect_from(self, emitter: Union[Emitter, Emitter.Handle]):
        """
        Disconnects from the given Emitter.
        :param emitter:  Emitter / Handle of the Emitter to disconnect from.
        """
        # get the emitter to disconnect from, if the input is a valid handle
        if isinstance(emitter, Emitter.Handle):
            emitter: Optional[Emitter] = emitter._element()
            if emitter is None:
                return

        # At this point the receiver might not even be connected to the emitter yet, if the connection was created as
        # part of the same event as this call to disconnect.
        # This is why we have to schedule a disconnection event and check then whether the connection exists at all.
        self.get_circuit().remove_connection(emitter, self)

    def disconnect_upstream(self):
        """
        Removes all upstream connections of this Receiver.
        """
        for emitter in self._upstream:
            self.get_circuit().remove_connection(emitter, self)

    def on_value(self, signal: ValueSignal):
        """
        Called when an upstream Emitter has produced a new Value.
        :param signal   The Signal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        assert any(emitter.get_id() == signal.get_source() for emitter in self._upstream)

        # it should be impossible to emit a value of the wrong type
        assert signal.get_value().schema == self.get_input_schema()

        # pass the signal along to the user-defined handler
        try:
            self._on_value(signal)
        except Exception as error:
            self.get_circuit().handle_error(
                Circuit.Error(weak_ref(self), Circuit.Error.Kind.USER_CODE_EXCEPTION, str(error)))

    def on_failure(self, signal: FailureSignal):
        """
        Called when an upstream Emitter has failed.
        :param signal       The ErrorSignal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        for candidate in self._upstream:
            if candidate.get_id() == signal.get_source():
                emitter: Emitter = candidate
                break
        else:
            assert False  # received Signal from an Emitter that is not connected

        # since the emitter has already completed, there is no reason to delay disconnecting
        # it will never emit again anyway
        self._upstream.remove(emitter)

        # mark the emitter as a candidate for deletion
        self.get_circuit().expire_emitter(emitter)

        # pass the signal along to the user-defined handler
        try:
            self._on_failure(signal)
        except Exception as error:
            self.get_circuit().handle_error(
                Circuit.Error(weak_ref(self), Circuit.Error.Kind.USER_CODE_EXCEPTION, str(error)))

    def on_completion(self, signal: CompletionSignal):
        """
        Called when an upstream Emitter has finished successfully.
        :param signal       The CompletionSignal associated with this call.
        """
        # assert that the emitter is connected to this receiver
        for candidate in self._upstream:
            if candidate.get_id() == signal.get_source():
                emitter: Emitter = candidate
                break
        else:
            assert False  # received Signal from an Emitter that is not connected

        # since the emitter has already completed, there is no reason to delay disconnecting
        # it will never emit again anyway
        self._upstream.remove(emitter)

        # mark the emitter as ready for deletion
        self.get_circuit().expire_emitter(emitter)

        # pass the signal along to the user-defined handler
        try:
            self._on_completion(signal)
        except Exception as error:
            self.get_circuit().handle_error(
                Circuit.Error(weak_ref(self), Circuit.Error.Kind.USER_CODE_EXCEPTION, str(error)))

    # private
    def _find_emitter(self, emitter_id: Emitter.ID) -> Optional[Emitter.Handle]:
        """
        Finds and returns an Emitter.Handle of any Emitter in the Circuit.
        :param emitter_id:  ID of the Emitter to find.
        :return:            A (non-owning) handle to the requested Emitter or NOne.
        """
        element: Optional[Circuit.Element] = self.get_circuit().get_element(emitter_id)
        if element is None or not isinstance(element, Emitter):
            return None
        return Emitter.Handle(element)

    def _on_value(self, signal: ValueSignal):  # virtual
        """
        Default implementation of the "value" method called whenever an upstream Emitter emitted a new Value.
        :param signal   The ValueSignal associated with this call.
        """
        pass

    def _on_failure(self, signal: FailureSignal):  # virtual
        """
        Default implementation of the "error" method called when an upstream Emitter has failed.
        By the time this function is called, the completed Emitter has already been disconnected.
        :param signal   The ErrorSignal associated with this call.
        """
        pass

    def _on_completion(self, signal: CompletionSignal):  # virtual
        """
        Default implementation of the "complete" method called when an upstream Emitter has finished successfully.
        By the time this function is called, the completed Emitter has already been disconnected.
        :param signal   The CompletionSignal associated with this call.
        """
        pass


########################################################################################################################

from .emitter import Emitter
from .operator import Operator
