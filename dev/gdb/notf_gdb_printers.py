"""
# gdb command to load notf pretty printers
python
import sys
sys.path.append("/home/clemens/code/notf/dev/gdb/")
import gdb.printing
import notf_gdb_printers
gdb.printing.register_pretty_printer(gdb.current_objfile(), notf_gdb_printers.build_pretty_printer())
end
"""
import gdb.printing


def build_vector_printer(dimensions, element_type):
    class Printer:
        def __init__(self, val):
            self.data = [val["data"]["__elems_"][i] for i in range(dimensions)]

        def children(self):
            return [item for item in zip("xyzw", self.data)]

        def display_hint(self):
            return "array"

        def to_string(self):
            value_fmt = "{}"
            for i in range(dimensions - 1):
                value_fmt += ", {}"
            return "V{}{}({})".format(dimensions, element_type, value_fmt.format(*self.data))
    return Printer


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("notf")

    pp.add_printer('notf::V2f', "^notf::detail::Vector2<float,.*>$",  build_vector_printer(2, "f"))
    pp.add_printer('notf::V2d', "^notf::detail::Vector2<double,.*>$", build_vector_printer(2, "d"))
    pp.add_printer('notf::V2i', "^notf::detail::Vector2<int,.*>$",    build_vector_printer(2, "i"))
    pp.add_printer('notf::V4s', "^notf::detail::Vector2<short,.*>$",  build_vector_printer(2, "s"))

    pp.add_printer('notf::V3f', "^notf::detail::Vector3<float,.*>$",  build_vector_printer(3, "f"))
    pp.add_printer('notf::V3d', "^notf::detail::Vector3<double,.*>$", build_vector_printer(3, "d"))
    pp.add_printer('notf::V3i', "^notf::detail::Vector3<int,.*>$",    build_vector_printer(3, "i"))
    pp.add_printer('notf::V3s', "^notf::detail::Vector3<short,.*>$",  build_vector_printer(3, "s"))

    pp.add_printer('notf::V4f', "^notf::detail::Vector4<float,.*>$",  build_vector_printer(4, "f"))
    pp.add_printer('notf::V4d', "^notf::detail::Vector4<double,.*>$", build_vector_printer(4, "d"))
    pp.add_printer('notf::V4i', "^notf::detail::Vector4<int,.*>$",    build_vector_printer(4, "i"))
    pp.add_printer('notf::V4s', "^notf::detail::Vector4<short,.*>$",  build_vector_printer(4, "s"))
    return pp
