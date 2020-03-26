from pynotf.data import Storage
from pynotf.logic import Circuit


# UTILITY ##############################################################################################################

class Application:

    def __init__(self):
        # Define the Data Storage.
        # This is where the entire state of the Application is stored in a named tuple of tables.
        self._data: Storage = Storage(
            circuit=Circuit.Data,
        )

        # Instead of operating on the tables directly, use a specialized object for each.
        self._circuit: Circuit = Circuit(self._data[0])

    def set_data(self, data):
        self._data.set_data(data)
        self._circuit.refresh_refcounts()

    def get_data(self):
        return self._data


THE_APPLICATION: Application = Application()
"""
The Application Singleton.
"""
