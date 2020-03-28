from pynotf.data import Storage
from pynotf.logic import Circuit


# UTILITY ##############################################################################################################

class Application:

    def __init__(self):
        # Define the Data Storage.
        # This is where the entire state of the Application is stored in a named tuple of tables.
        self._storage: Storage = Storage(
            circuit=Circuit.EmitterColumns,  # table index: 0
        )

        self._circuit: Circuit = Circuit(self._storage)
