import dis
import inspect


class Property:
    def __init__(self, name, value):
        self._name = name
        self._value = value
        self._expression = None

    def get_value(self):
        return self._value

    def set_expression(self, expr):
        print(extract_properties(expr))
        self._expression = expr

    def __repr__(self):
        return "Property({}, {})".format(self._name, self._value)


def extract_properties(func):
    # find the starting scope of `func`
    if inspect.isfunction(func):
        if func.__name__ == "<lambda>":
            scope = func.__closure__[0].cell_contents.__dict__
        else:
            scope = globals()
    elif inspect.ismethod(func):
        scope = func.__self__.__dict__
    else:
        raise ValueError("Unsupported expression type of \"{}\"".format(func.__name__))

    def extract_object(from_line):
        # find the first line where `func` starts accessing the property
        stylus = from_line
        while True:
            stylus -= 1
            if bytecode[stylus].opcode == 106:
                continue
            elif bytecode[stylus].opcode == 116:
                stylus -= 1
            break
        stylus += 1

        # extract the result object
        obj = scope[bytecode[stylus].argval]
        stylus += 1
        while stylus < from_line:
            obj = getattr(obj, bytecode[stylus].argval)
            stylus += 1
        return obj

    properties = set()
    bytecode = [instr for instr in dis.get_instructions(func)]
    for line in range(len(bytecode)):
        instruction = bytecode[line]

        # find Properties when they are loaded
        if instruction.opcode == 106:
            candidate = extract_object(line)
            if isinstance(candidate, Property):
                properties.add(candidate)

        # the function can also contain nested functions
        elif instruction.opcode == 131:
            nested_func = extract_object(line)
            if inspect.ismethod(nested_func) or inspect.isfunction(nested_func):
                properties.update(extract_properties(nested_func))

    properties.discard(None)
    return properties


global_foo_1 = Property("global_foo_1", 1)
global_foo_2 = Property("global_foo_2", 2)


def free_func_using_globals():
    return global_foo_1.get_value() + global_foo_2.get_value()


class Controller1:
    def __init__(self):
        self.prop_a = Property("controller1_a", 1)
        self.prop_b = Property("controller1_b", 2)
        self.prop_c = Property("controller2_c", 0)

        # self.prop_c.set_expression(lambda: self.prop_a.get_value() + self.prop_b.get_value())  # works
        # self.prop_c.set_expression(self.method)  # works

    def method(self):
        return self.prop_a.get_value() + self.prop_b.get_value()


c = Controller1()


class Container:
    def __init__(self):
        self.c = Controller1()

    def get_property(self):
        return self.c.prop_a.get_value()

container = Container()


def free_func_using_controller():
    return c.prop_a.get_value() + c.prop_b.get_value()


global_foo_1.gv = global_foo_1.get_value;


def free_func_using_container():
    return container.c.prop_a.get_value() + container.c.prop_b.get_value() + global_foo_2.get_value() + c.prop_b.get_value() + global_foo_1.gv()


def stupidfunc():
    return global_foo_1.get_value()


def using_free_fun():
    return global_foo_2.get_value() + stupidfunc()


def using_method():
    return container.get_property()


def from_dict():
    evildict = {"foo": global_foo_1}
    return evildict["foo"].get_value()

def using_confuscation():
    blub = global_foo_1
    return blub.get_value()

if __name__ == "__main__":
    a = Property("a", 0)
    # a.set_expression(free_func_using_globals)  # works
    # a.set_expression(free_func_using_container)  # works
    # a.set_expression(using_free_fun)  # works
    # a.set_expression(using_method)  # works
    # a.set_expression(from_dict) # DOES NOT WORK
    a.set_expression(using_confuscation)
