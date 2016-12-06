from notf import *

class Blub(Foo):
    def __init__(self):
        super().__init__()
        globals()["__object_cache"].add(self)

    def do_foo(self):
        print("Blub foo")

def produce_foos():
    bar = Bar()
    blub = Blub()
    add_foo(bar)
    add_foo(blub)

def main():
    produce_foos()

if __name__ == "__main__":
    main()
