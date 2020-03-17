from pynotf.data import Storage
from pynotf.logic import Circuit2


# UTILITY ##############################################################################################################

class Application:

    def __init__(self):
        # Define the Data Storage.
        # This is where the entire state of the Application is stored in a named tuple of tables.
        self._data: Storage = Storage(
            circuit=Circuit2.Data,
        )

        # Instead of operating on the tables directly, use a specialized object for each.
        self._circuit: Circuit2 = Circuit2(self._data["circuit"])
