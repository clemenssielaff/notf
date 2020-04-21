from typing import Tuple, Sized


def chunks(iterable: Sized, size: int):
    """
    Yield successive n-sized chunks from a list or other "sliceable".
    :param iterable: List or other iterable that supports slicing.
    :param size: Maximum number of elements in a chunk. Must not be zero, negative size reverses the iteration.
    :returns: The next chunk in the given iterable.
    """
    assert size != 0
    if size > 0:
        for i in range(0, len(iterable), size):
            yield iterable[i:i + size]
    else:
        for i in range(1, len(iterable) + 1, -size):
            yield iterable[-i:-i + size:-1]


def color_text(text: str, rgb: Tuple[int, int, int]) -> str:
    """
    Colors the given text for display in the terminal.
    :param text: Text to colorize.
    :param rgb: RGB values for the color (0-255).
    :return: Colorized text.
    """
    return f'\033[38;2;{rgb[0]};{rgb[1]};{rgb[2]}m{text}\033[0m'


def color_background(text: str, rgb: Tuple[int, int, int]) -> str:
    """
    Colors the background of the given text for display in the terminal.
    :param text: Text to colorize.
    :param rgb: RGB values for the color (0-255).
    :return: Colorized text.
    """
    return f'\033[48;2;{rgb[0]};{rgb[1]};{rgb[2]}m{text}\033[0m'
