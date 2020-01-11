from typing import Any

from pynotf.value import Value
from pynotf.logic import Circuit, Emitter


#######################################################################################################################

class OutputPlug(Emitter):
    """
    Outward facing Emitter on a Widget.
    """

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, schema: Value.Schema):
        """
        Constructor.
        It should only be possible to create-connect an InputPlug when constructing a new Widget instance.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param schema:      Value type accepted by this Plug.
        """
        Emitter.__init__(self, schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant

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

    def emit(self, value: Any):
        """
        This method should only be callable from within Widget Callbacks.
        If the given Value does not match the output Schema of this Emitter, an exception is thrown. This is so that the
        Callback is interrupted and the ValueSignal remains unchanged. The exception will be caught in the IPlug
        `_on_value` function and handled by the Circuit's error handler.
        :param value:       Value to emit.
        :raise TypeError:   If the given value does not match the output Schema of this Emitter.
        """
        # implicit value conversion
        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError:
                raise TypeError("Emitters can only emit Values or things that are implicitly convertible to one")

        # ensure that the value can be emitted by the Emitter
        if self.get_output_schema() != value.schema:
            raise TypeError(f"Cannot emit Value from Emitter {self.get_id()}."
                            f"  Emitter schema: {self.get_output_schema()}"
                            f"  Value schema: {value.schema}")

        self._emit(value)
