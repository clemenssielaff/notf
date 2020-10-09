from __future__ import annotations

from ctypes import cast as c_cast, c_void_p
from enum import IntEnum, unique, auto
from typing import Dict

import glfw
from pycnotf import V2f

from pynotf.data import Value
from pynotf.core import IndexEnum
from pynotf.extra import set_bit


# UTILS ################################################################################################################

@unique
class MouseButtons(IntEnum):
    """
    Button indices, all different bits so you can create a 8-bit bitset to contain them all.
    """
    LEFT = 1 << 0
    RIGHT = 1 << 1
    MIDDLE = 1 << 2
    BUTTON_4 = 1 << 3
    BUTTON_5 = 1 << 4
    BUTTON_6 = 1 << 5
    BUTTON_7 = 1 << 6
    BUTTON_8 = 1 << 7


class ButtonAction(IndexEnum):
    """
    Button actions, same as in GLFW.
    """
    RELEASE = auto()
    PRESS = auto()
    REPEAT = auto()


class ModifierKeys(IntEnum):
    """
    Active modifier keys, same as in GLFW.
    """
    SHIFT = 1 << 0
    CTRL = 1 << 1
    ALT = 1 << 2
    SUPER = 1 << 3
    CAPS_LOCK = 1 << 4
    NUM_LOCK = 1 << 5


# IO SERVICE ###########################################################################################################

class IoService:
    """
    The IO Service manages two facts: `mouse_buttons` and `mouse_position`.

    `mouse_buttons`
    ===============
    Is updated whenever a mouse button is pressed or released.
    The Value produced by mouse clicks is set up as follows:
    {
        pos: {x:384, y:687},
        action: ButtonAction.PRESS,
        button: 1,  # MouseButtons.LEFT
        buttons: 3,  # MouseButtons.LEFT | MouseButtons.RIGHT
        modifiers: 2,  # ModifierKeys.CTRL
    }
    Initial = Value(pos=dict(x=0, y=0), action=0, button=0, buttons=0, modifiers=0)
    Schema = [0, 5, 5, 253, 253, 253, 253, 0, 2, 253, 253]

    `mouse_position`
    ================
    Is updated whenever the mouse cursor moves over a window, or during a drag initiated from one.
    If a modifier key is pressed outside the window and the mouse moves into the window, there is no way for us to
    identify the modifier until it is either released or a mouse button is clicked.
    The Value produced by mouse movement is set up as follows:
    {
        pos: {x:384, y:687},
        delta: {x:0, y:-2},
        buttons: 3,  # MouseButtons.LEFT | MouseButtons.RIGHT
        modifiers: 2,  # ModifierKeys.CTRL
    }
    Initial = Value(pos=dict(x=0, y=0), delta=dict(x=0, y=0), buttons=0, modifiers=0)
    Schema = [0, 4, 4, 7, 253, 253, 0, 2, 253, 253, 0, 2, 253, 253]
    """

    class _WindowData:
        """
        Per-Window data.
        """
        __slots__ = ('was_outside', 'last_pos', 'pressed_buttons', 'pressed_modifiers')

        _windows: Dict[int, IoService._WindowData] = {}

        def __init__(self):
            self.was_outside: bool = True
            self.last_pos: V2f = V2f.zero()
            self.pressed_buttons: int = 0  # bitset
            self.pressed_modifiers: int = 0  # bitset

        @classmethod
        def get(cls, window) -> IoService._WindowData:
            return cls._windows.setdefault(c_cast(window, c_void_p).value, cls())

    _scene = None

    @classmethod
    def initialize(cls, scene, window) -> None:
        cls._scene = scene
        glfw.set_mouse_button_callback(window, cls.mouse_button_callback)
        glfw.set_cursor_pos_callback(window, cls.cursor_position_callback)
        glfw.set_cursor_enter_callback(window, cls.cursor_enter_callback)
        glfw.set_key_callback(window, cls.key_callback)

    @classmethod
    def mouse_button_callback(cls, window, glfw_button: int, glfw_action: int, glfw_mods: int) -> None:
        """
        Buttons that are pressed outside the window before the mouse moved in, are not known to be pressed and will not
        show up in the `buttons` field.
        :param window: Active window.
        :param glfw_button: Mouse button triggering this callback.
        :param glfw_action: Is one of glfw.PRESS or glfw.RELEASE.
        :param glfw_mods: Active modifier keys.
        """
        button: int = 1 << glfw_button

        # Update the window data
        # It is important to set the `pressed_buttons` field, not to toggle it as we might miss keystrokes that happen
        # outside the Window.
        window_data: cls._WindowData = cls._WindowData.get(window)
        window_data.pressed_buttons = set_bit(window_data.pressed_buttons, button, bool(glfw_action))
        window_data.pressed_modifiers = glfw_mods

        cls._scene.get_fact('mouse_buttons').update(Value(
            pos=dict(x=window_data.last_pos.x, y=window_data.last_pos.y),
            action=glfw_action,
            button=button,
            buttons=window_data.pressed_buttons,
            modifiers=window_data.pressed_modifiers,
        ))

    @classmethod
    def cursor_position_callback(cls, window, x: float, y: float) -> None:
        """
        Is called whenever the mouse is moved inside the window.
        If you click the mouse and drag the cursor out of the window, it will still receive new position values that lie
        outside the window rectangle (negative values are possible).
        :param window: Active window.
        :param x: Subpixel horizontal coordinate of the mouse cursor relative to the top left corner of the Window.
        :param y: Subpixel vertical coordinate of the mouse cursor relative to the top left corner of the Window.
        """
        new_pos: V2f = V2f(x, y)
        window_data: cls._WindowData = cls._WindowData.get(window)
        delta: V2f = V2f.zero() if window_data.was_outside else new_pos - window_data.last_pos

        # update the window data
        window_data.last_pos = new_pos
        window_data.was_outside = False

        cls._scene.get_fact('mouse_position').update(Value(
            pos=dict(x=x, y=y),
            delta=dict(x=delta.x, y=delta.y),
            buttons=window_data.pressed_buttons,
            modifiers=window_data.pressed_modifiers,
        ))

    @classmethod
    def cursor_enter_callback(cls, window, entered: int) -> None:
        """
        Is called whenever the cursor leaves or enters the window.
        :param window: Active window.
        :param entered: 1 if the cursor entered the Window 0 if it left.
        """
        if entered == 0:
            window_data: cls._WindowData = cls._WindowData.get(window)
            window_data.was_outside = True
        else:
            # we don't care about window enter events, the position callback handles those
            pass

    @classmethod
    def key_callback(cls, window, key: int, scancode: int, action: int, mods: int) -> None:
        # TODO: keyboard callback
        if action not in (glfw.PRESS, glfw.REPEAT):
            return

        if glfw.KEY_A <= key <= glfw.KEY_Z:
            cls._scene.get_scene().get_fact('key_fact').update(Value())
