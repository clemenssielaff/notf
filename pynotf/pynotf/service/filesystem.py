from typing import Union

from pynotf.core import Service

import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler, FileSystemEvent, DirMovedEvent, FileMovedEvent, DirModifiedEvent, \
    FileModifiedEvent, DirCreatedEvent, FileCreatedEvent, DirDeletedEvent, FileDeletedEvent


class Handler(FileSystemEventHandler):
    def on_any_event(self, event: FileSystemEvent) -> None:
        """
        Is called on every change
        """
        pass

    def on_created(self, event: Union[DirCreatedEvent, FileCreatedEvent]) -> None:
        kind: str = "directory" if isinstance(event, DirCreatedEvent) else "file"
        print(f'Created {kind} "{event.src_path}"')

    def on_moved(self, event: Union[DirMovedEvent, FileMovedEvent]) -> None:
        kind: str = "directory" if isinstance(event, DirMovedEvent) else "file"
        print(f'Moved {kind} "{event.src_path}" to "{event.dest_path}"')

    def on_modified(self, event: Union[DirModifiedEvent, FileModifiedEvent]) -> None:
        kind: str = "directory" if isinstance(event, DirModifiedEvent) else "file"
        print(f'Modified {kind} "{event.src_path}"')

    def on_deleted(self, event: Union[DirDeletedEvent, FileDeletedEvent]) -> None:
        kind: str = "directory" if isinstance(event, DirDeletedEvent) else "file"
        print(f'Deleted {kind} "{event.src_path}"')


if __name__ == "__main__":
    path = r'/home/clemens/temp/notf/'
    event_handler = Handler()
    observer = Observer()
    observer.schedule(event_handler, path, recursive=True)
    observer.start()
    try:
        print(f'Started observing "{path}"')
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
        print(f'Stopped observing "{path}"')
    observer.join()
