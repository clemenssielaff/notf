from threading import current_thread

RENDER_THREAD_NAME = 'notf_render_thread'


def this_is_the_render_thread():
    """
    Checks if the current thread is the notf render thread.
    :return:
    """
    return current_thread().name == RENDER_THREAD_NAME
