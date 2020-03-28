from typing import Optional

from pynotf.data import Value, RowHandle, HandleError, Table
from pynotf.core import get_app

from .signals import CompletionSignal, FailureSignal, ValueSignal


########################################################################################################################

class Receiver:
    """
    Non-owning object operating on a Circuit table row that contains a Receiver.
    """

    # def create_operator(self, operation: 'Operator.Operation') -> Optional['Operator.CreatorHandle']:  # noexcept
    #     """
    #     Creates and connects to a new Operator upstream.
    #     :param operation: Operation defining the Operator.
    #     :returns: New Operator or None if the output type of the Operation does not match this Receiver's.
    #     """
    #     # fail immediately if the schemas don't match
    #     if self.get_input_schema() != operation.get_output_schema():
    #         return self.get_circuit().handle_error(Circuit.Error(
    #             weak_ref(self),
    #             Circuit.Error.Kind.WRONG_VALUE_SCHEMA,
    #             f"Cannot create an Operator with the output Value Schema {operation.get_output_schema()} "
    #             f"from a Receiver with the input Value Schema {self.get_input_schema()}"))
    #
    #     # create the Operator
    #     operator: Operator = self.get_circuit().create_element(Operator, operation)
    #
    #     # store the operator right away to keep it alive, this does not count as a connection yet
    #     self._upstream.add(operator)
    #
    #     # the actual connection is delayed until the end of the event
    #     self.get_circuit().create_connection(operator, self)
    #
    #     # return a handle to the freshly created Operator
    #     return Operator.CreatorHandle(operator)

    def connect_to(self, emitter: RowHandle):  # noexcept
        """
        Connect to the given Emitter.
        If the Emitter is invalid or this Receiver is already connected to it, this does nothing.
        :param emitter: Handle of the Emitter to connect to.
        """
        table: Table = get_app().get_circuit().get_table()

        # check if the emitter is valid
        if not table.is_handle_valid(emitter):
            return

        # do nothing if the emitter is already connected
        if emitter.index in self._get_row()['upstream']:
            return

        # fail immediately if the schemas don't match
        if self.get_input_schema() != emitter.get_output_schema():
            return self.get_circuit().handle_error(Circuit.Error(
                weak_ref(self),
                Circuit.Error.Kind.WRONG_VALUE_SCHEMA,
                f"Cannot connect Emitter {emitter.get_id()} "
                f"with the output Value Schema {emitter.get_output_schema()} "
                f"to a Receiver with the input Value Schema {self.get_input_schema()}"))

        # store the emitter right away to keep it alive
        # as the emitter does not know about this, it does not count as a connection yet
        self._get_row()['upstream'] = self._get_row()['upstream'].append(emitter.index)

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
        assert signal.get_value().get_schema() == self.get_input_schema()

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
