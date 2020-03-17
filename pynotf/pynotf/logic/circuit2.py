from typing import Type, Dict

from pynotf.data import Value, RowHandleList, Table


class Circuit2:

    # All data stored in a single row of the Circuit table.
    Data: Dict[str, Type] = dict(
        input_schema=Value.Schema,  # The Schema of Values received from upstream (empty if unused).
        # Is only used for Receivers. If the Element is a pure Emitter, this Schema is empty. Note however that you cannot
        # conclude the other way: If the input Schema is None, the Element can still be a Receiver, just one that receives
        # None Values exclusively.

        output_schema=Value.Schema,  # The Schema of Values emitted downstream (empty if unused).
        # Is only used for Emitters. If the Element is a pure Receiver, this Schema is empty. Note however that you cannot
        # conclude the other way: If the input Schema is None, the Element can still be an Emitter, just one that emits
        # None Values exclusively.

        upstream=RowHandleList,  # unique connected emitters upstream
        downstream=RowHandleList,  # unique connected receivers downstream

        value=Value,  # The last emitted Value.
        # The last emitted Value is only relevant for Emitters. It has the same Schema as is stored in output_schema, and
        # is undefined (actually: zero initialized) if the Emitter has not emitted anything yet.

        status=int,  # 8-bit Element bitset:
        # | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
        #   +---+---+   |   X   |   |   |
        #       |       |       |   |  is emitter?
        #       |       |       |  is receiver?
        #       |       |      is blockable?
        #       |      has emitted?
        #      emitter status
    )

    def __init__(self, circuit_table: Table):
        assert circuit_table.keys() == list(self.Data.keys())  # make sure we are handed the correct table
        self._table: Table = circuit_table
