from notf import Foo, Bar, add_foo

class Blub(Foo):
    def do_foo(self):
        print("Blub foo")

class Brap(Foo):
    def do_foo(self):
        print("Brap foo")

def produce_foos():
    bar = Bar()
    blub = Blub()
    brap = Brap()
    add_foo(bar)
    #add_foo(blub)
    add_foo(brap)

def main():
    pass
    produce_foos()

if __name__ == "__main__":
    main()
