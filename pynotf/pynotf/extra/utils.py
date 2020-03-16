from typing import Tuple


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
