from logging import error as print_error

from pynotf.data import Storage
import pynotf.core as core


# UTILITY ##############################################################################################################

class Application:

    def __init__(self):
        # Define the Data Storage.
        # This is where the entire state of the Application is stored in a named tuple of tables.
        self._storage: Storage = Storage(
            circuit=core.Circuit.EmitterColumns,  # table index: 0
        )

        self._circuit: core.Circuit = core.Circuit(self)

    def get_storage(self) -> Storage:
        return self._storage

    @staticmethod
    def handle_error(error: core.Error) -> None:
        print_error(f'[error {error.kind}] {error.message}')
