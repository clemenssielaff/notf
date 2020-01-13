from __future__ import annotations
from typing import Optional, Any

from pynotf.value import Value
from pynotf.callback import Callback as CallbackBase
from pynotf.logic import Circuit, ValueSignal, Emitter, Receiver


#######################################################################################################################

class Property(Receiver, Emitter):  # final

    class Operation(CallbackBase):
        """
        A Property Operator is a Callback that is run on every new value that the Property receives via `_on_value`.

        The signature of a Property.Operation is as follows:

            (value: Value, widget: Widget.View, data: Value) -> (Optional[Value], Optional[Value])

        Parameter:
            value:  The new Value to operate on.
            widget: Widget.View object to inspect other Properties on the same Widget.
            data:   Private instance data that can be modified by the Operation and will be passed back the next time.

        Return type:
            first:  New Value of the Property or None to retain the existing one.
            second: New data to store in the Operation or None to retain the existing data.
        """

        def __init__(self, source: str, data: Optional[Value] = None):
            """
            Constructor.
            :param source:  Function body of a function adhering to the signature of a Property.Operation.
            :param data:    Initial private data Value. Is empty by default.
            """
            CallbackBase.__init__(self, dict(value=Value, widget=Widget.View, data=Value), source, dict(Value=Value))

            # initialize an empty data Value that can be used by the Callback function to store state
            self._data: Value = data or Value()

        def __call__(self, value: Value, widget: Widget.View) -> Optional[Value]:
            """
            Wraps Callback.__call__ to add the private data argument.
            :param value:       New value for the Callback to operate on.
            :param widget:      Widget.View object to inspect other Properties on the same Widget.
            :return:            Callback result.
            :raise TypeError:   If the callback did not return the correct result types.
            :raise Exception: From user-defined code.
            """
            # execute the Callback and add the private data as argument
            result = CallbackBase.__call__(self, value, widget, self._data)  # this could throw any exception

            # the return type should be a Value, Value pair
            if not (isinstance(result, tuple) and len(result) == 2):
                raise TypeError("Property.Callbacks must return (Optional[Value], Optional[Value])")
            new_value, new_data = result
            # TODO: with mutable Values, we should be able to modify the data argument directly (and not return it)

            # check the returned value
            if new_value is not None and not isinstance(new_value, Value):
                try:
                    new_value = Value(new_value)
                except ValueError:
                    raise TypeError("Property.Callbacks must return the new value or None as the first return value")

            # update the private data
            if new_data is not None:
                success = True
                if not isinstance(new_data, Value):
                    try:
                        new_data = Value(new_data)
                    except ValueError:
                        success = False
                if success and new_data.schema != self._data.schema:
                    success = False
                if not success:
                    raise TypeError("Property.Callbacks must return the data or None as the second return value")
                else:
                    self._data = new_data

            return new_value

    class Callback(CallbackBase):
        """
        Callback invoked by a Property after its Value changed.
        """

        def __init__(self, source: str):
            CallbackBase.__init__(self, dict(value=Value, widget=Widget.Handle), source)

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, widget: 'Widget', value: Value,
                 operation: Optional[Operation]):
        """
        Constructor.
        Should only be called from a Widget constructor with a valid Widget.Definition - hence we don't check the
        operation for validity at runtime and just assume that it ingests and produces the correct Value type.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param widget: Widget that this Property lives on.
        :param value: Initial value of the Property, also defines its Schema.
        :param operation: Operations applied to every new Value of this Property, can be a noop.
        """
        # various asserts, all of this should be guaranteed by the Widget.Definition
        assert not value.schema.is_empty()

        Receiver.__init__(self, value.schema)
        Emitter.__init__(self, value.schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._operation: Optional[Property.Operation] = operation  # is constant
        self._widget: Widget = widget  # is constant
        self._callback: Optional[Property.Callback] = None
        self._value: Value = value

    def get_id(self) -> Circuit.Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> Circuit:  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    def get_value(self) -> Value:  # noexcept
        """
        The current value of this Property.
        """
        return self._value

    def set_callback(self, callback: Optional[Property.Callback]):
        """
        Callbacks must take two arguments (Value, Widget.Handle) and return nothing.
        :param callback:    New Callback to call when the Property changes its Value.
        """
        self._callback = callback

    def set_value(self, value: Any):
        """
        Applies a new Value to this Property. The Value must be of the Property's type and is transformed by the
        Property's Operation before assignment.
        :param value: New value to assign to this Property.
        :raise TypeError: If the given Value type does not match the one stored in this Property.
        :raise Exception: From user-defined code.
        """
        if value is None:
            raise TypeError("Property values cannot be none")

        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError as exception:
                raise TypeError(str(exception)) from exception
        assert isinstance(value, Value)

        if value.schema != self._value.schema:
            raise TypeError("Cannot change the type of a Property")

        # since the Operation contains user-defined code, this might throw
        if self._operation is not None:
            value = self._operation(value, Widget.View(self._widget))
            assert value.schema == self._value.schema

        # return early if the value didn't change
        if value == self._value:
            return

        # update the property's value
        self._value = value

        # invoke the registered callback, if there is one
        if self._callback is not None:
            self._callback(self._value, Widget.Handle(self._widget))

        # lastly, emit the changed value into the circuit
        self._emit(self._value)

    def _on_value(self, signal: ValueSignal):
        """
        Abstract method called by any upstream Emitter.
        :param signal   The ValueSignal associated with this call.
        """
        self.set_value(signal.get_value())


#######################################################################################################################

from .widget import Widget
