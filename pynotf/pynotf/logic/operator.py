from __future__ import annotations
from abc import ABC, abstractmethod
from typing import Optional
from weakref import ref as weak_ref

from ..value import Value

from .receiver import Receiver
from .emitter import Emitter
from .circuit import Circuit
from .signals import ValueSignal, FailureSignal, CompletionSignal


########################################################################################################################

class Operator(Receiver, Emitter):  # final
    """
    Operators are use-defined functions in the Circuit that take a Value and maybe produce one. Not all input values
    generate an output value though.
    Operators will complete automatically once all upstream Emitters have completed or through failure.
    """

    class Operation(ABC):
        """
        Operations are Functors that define an Operator.
        """

        @abstractmethod
        def get_input_schema(self) -> Value.Schema:
            """
            The Schema of an input value of the Operation.
            """
            pass

        @abstractmethod
        def get_output_schema(self) -> Value.Schema:
            """
            The Schema of an output value of the Operation.
            """
            pass

        @abstractmethod
        def __call__(self, value: Value) -> Optional[Value]:
            """
            Perform the Operation on a given value.
            If this function returns a Value, it will be emitted by the Operator.
            If this function returns None, no emission will take place.
            Exceptions thrown by this function will cause the Operator to fail.
            :param value: Input value, conforms to the input Schema.
            :return: Either a new output value conforming to the output Schema or None.
            """
            raise NotImplementedError()

    class CreatorHandle(Emitter.Handle):
        """
        A special kind of handle that is returned when you create an Operator from a Receiver.
        It allows you to continue the chain of Operators upstream if you want to split your calculation into multiple,
        re-usable parts.
        """

        def __init__(self, operator: 'Operator'):
            """
            :param operator: Strong reference to the Operator represented by this handle.
            """
            Emitter.Handle.__init__(self, operator)

        def connect_to(self, emitter_handle: Emitter.Handle):
            """
            Connects this newly created Operator to an existing Emitter upstream.
            Does nothing if the Handle has expired.
            :param emitter_handle:  Emitter to connect to. If this Receiver is already connected, this does nothing.
            """
            operator: Optional[Operator] = self._element()
            if operator is not None:
                assert isinstance(operator, Operator)
                operator.connect_to(emitter_handle)

        def create_operator(self, operation: Operator.Operation) -> Optional[Operator.CreatorHandle]:  # noexcept
            """
            Creates and connects this newly created Operator to another new Operator upstream.
            Does nothing if the Handle has expired.
            :param operation: Operation defining the Operator.
            """
            operator: Optional[Operator] = self._element()
            if operator is not None:
                assert isinstance(operator, Operator)
                return operator.create_operator(operation)

    def __init__(self, circuit: 'Circuit', element_id: Circuit.Element.ID, operation: Operation):
        """
        Constructor.
        It should only be possible to create-connect an Operator from a Receiver.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param operation:   The Operation performed by this Operator.
        """
        Receiver.__init__(self, operation.get_input_schema())
        Emitter.__init__(self, operation.get_output_schema())

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._operation: Operator.Operation = operation  # is constant

    def get_id(self) -> Circuit.Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> 'Circuit':  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    # private
    def _on_value(self, signal: ValueSignal):  # final
        """
        Performs the Operation on the given Value and emits the result if one is produced.
        Exceptions thrown by the Operation will cause the Operator to fail.
        :param signal   The ValueSignal associated with this call.
        """
        value: Value = signal.get_value()
        assert value.schema == self.get_input_schema()

        # invoke the operation to produce a result or an error
        error: Optional[Exception] = None
        result: Optional[Value] = None
        try:
            result = self._operation(value)
        except Exception as exception:
            error = exception
        else:
            # if no error occurred, but no result was produced either, there's nothing left to do
            if result is None:
                return
            elif result.schema != self.get_output_schema():
                error = TypeError("Return Value of Operation does not conform to the Operator's output Schema")

        # if any error occurred, the Operator will complete through failure
        if error is not None:
            self.get_circuit().handle_error(
                Circuit.Error(weak_ref(self), Circuit.Error.Kind.USER_CODE_EXCEPTION, str(error)))
            self._fail(error)

        # if a valid result was produced successfully, emit it
        else:
            self._emit(result)

    def _on_failure(self, signal: FailureSignal):  # final
        """
        If the failed Emitter was the last connected one, this Operator also completes.
        :param signal   The ErrorSignal associated with this call.
        """
        if not self.has_upstream():
            self._complete()

    def _on_completion(self, signal: CompletionSignal):  # final
        """
        If the completed Emitter was the last connected one, this Operator also completes.
        :param signal   The CompletionSignal associated with this call.
        """
        if not self.has_upstream():
            self._complete()
