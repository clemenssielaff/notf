from notf import *

class TestPainter(Painter):

    def paint(self):
        print("I'm painting!")
#        self.start_path();
        #set_fill(Color(1, 1, 1.f));
#        self.circle(50, 50, 50);
#        self.fill();
#        furz
#        blubdibla

    def print_name(self):
        print("Python painter")
        

def main():
    painter = TestPainter()

    canvas = CanvasComponent()
    canvas.set_painter(painter)

    circle = Widget()
    circle.add_component(canvas)

    window = Window()
    window.get_layout_root().set_item(circle)

    cPainter = Painter()
    name_painter(cPainter)
    name_painter(painter)
    print("that's all")

if __name__ == "__main__":
    main()
