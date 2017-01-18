import build_notf
from notf import *

from std import Button


class WindowController(Controller):
    def __init__(self):
        super().__init__()
        window_layout = Overlayout()
        window_layout.set_alignment(Overlayout.Alignment(int(Overlayout.Alignment.LEFT) | int(Overlayout.Alignment.VCENTER)))
        self.set_root_item(window_layout)

        window_layout.add_item(Button("derbness"))


def main():
    window = Window()
    window_controller = WindowController()
    window.get_layout_root().set_item(window_controller)


if 0:
    build_notf.produce()

if __name__ == "__main__":
    main()
