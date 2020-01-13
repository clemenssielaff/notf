from __future__ import annotations
from typing import Optional
from weakref import ref as weak_ref

from pynotf.value import Value
from pynotf.callback import Callback as CallbackBase
from pynotf.logic import Circuit, ValueSignal, Receiver


#######################################################################################################################

class InputPlug(Receiver):
    """
    An Input Plug (IPlug) accepts Values from the Circuit and forwards them to an optional Callback on its Widget.
    """

    class Callback(CallbackBase):
        """
        Callback invoked by an InputPlug.
        """

        def __init__(self, source: str):
            CallbackBase.__init__(self, dict(signal=ValueSignal, widget=Widget.Handle), source)

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, widget: 'Widget', schema: Value.Schema):
        """
        Constructor.
        It should only be possible to create-connect an InputPlug when constructing a new Widget instance.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param widget:      Widget on which the Plug lives.
        :param schema:      Value type accepted by this Plug.
        """
        Receiver.__init__(self, schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._widget: Widget = widget  # is constant
        self._callback: Optional[InputPlug.Callback] = None

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

    def set_callback(self, callback: Optional[InputPlug.Callback]):
        """
        Callbacks must take two arguments (ValueSignal, Widget.Handle) and return nothing.
        :param callback:    New Callback to call when a new Value is received or None to uninstall the Callback.
        """
        self._callback = callback

    # private
    def _on_value(self, signal: ValueSignal):  # final
        """
        Forwards the Signal unmodified to the registered Callback or ignores it, if no Callback is set.
        Exceptions in the Callback will be reported and the Signal will remain unmodified, even if the Callback accepted
        or blocked it prior to failure.
        :param signal   The ValueSignal associated with this call.
        """
        assert signal.get_value().schema == self.get_input_schema()

        # don't do anything if no Callback is registered
        if self._callback is None:
            return

        # create a copy of the Signal so the original remains unmodified if the Callback fails
        signal_copy: ValueSignal = ValueSignal(signal.get_source(), signal.get_value(), signal.is_blockable())
        if signal.is_blocked():
            signal_copy.block()
        elif signal.is_accepted():
            signal.accept()

        # perform the callback
        try:
            self._callback(Widget.Handle(self._widget), signal_copy)

        # report any exception that might escape the user-defined code
        except Exception as exception:
            self.get_circuit().handle_error(Circuit.Error(
                weak_ref(self),
                Circuit.Error.Kind.USER_CODE_EXCEPTION,
                str(exception)
            ))

        # only modify the original signal if the callback succeeded
        else:
            if signal.is_blockable() and not signal.is_blocked():
                if signal_copy.is_blocked():
                    signal.block()
                elif signal_copy.is_accepted():
                    signal.accept()  # accepting more than once does nothing


#######################################################################################################################

from .widget import Widget
