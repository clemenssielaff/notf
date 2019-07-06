from threading import current_thread
from enum import Enum, unique, auto


@unique
class NotfThreadKind(Enum):
    """
    Thread identifiers.
    """
    MAIN = auto()
    RENDER = auto()


def is_this_the_render_thread():
    """
    Checks if the current thread is the notf render thread.
    """
    return getattr(current_thread(), 'notf_type', None) == NotfThreadKind.RENDER


def is_this_the_main_thread():
    """
    Checks if the current thread is the notf render thread.
    """
    kind = getattr(current_thread(), 'notf_type', None)
    return kind is None or kind == NotfThreadKind.MAIN
