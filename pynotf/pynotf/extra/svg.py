from typing import NamedTuple, List, Optional
import xml.etree.ElementTree as Xml
from re import compile as compile_regex

from pycnotf import V2f
from pynotf.data.shape import Shape, ShapeBuilder
from pynotf.extra.utils import chunks

# from https://www.w3.org/TR/SVG11/single-page.html#intro-NamespaceAndDTDIdentifiers
SVG_NAMESPACE: str = 'http://www.w3.org/2000/svg'


# from https://www.w3.org/TR/SVGTiny12/paths.html#PathData
class PathDescription(NamedTuple):
    name: str
    shapes: List[Shape] = []


COMMAND_LETTERS = compile_regex(r'\s*([mM]|[lL]|[hH]|[vV]|[cC]|[sS]|[qQ]|[tT]|[zZ])\s*')
COORDINATE_SEPARATOR = compile_regex(r'\s+,\s*|,\s*')
NUMBER = compile_regex(r'[+-]?(?:\d*\.\d+|\d+\.)(?:[eE][+-]?\d*)?|[+-]?(?:\d+)')
COORDINATES = compile_regex(
    r'([+-]?(?:\d*\.\d+|\d+\.)(?:[eE][+-]?\d*)?|[+-]?(?:\d+))(?:\s+,\s*|,\s*)?([+-]?(?:\d*\.\d+|\d+\.)(?:[eE][+-]?\d*)?|[+-]?(?:\d+))')


def parse_shapes(definition: str) -> List[Shape]:
    result: List[Shape] = []
    current_command: Optional[str] = None
    last_ctrl_point: Optional[V2f] = None
    current_coordinate: V2f = V2f(0, 0)
    current_shape: ShapeBuilder = ShapeBuilder(current_coordinate)
    for line in (line for line in COMMAND_LETTERS.split(definition) if line):
        # determine the current command
        if current_command is None:
            current_command = line
            continue
        elif line == current_command:
            continue  # repeating the same command letter is optional
        assert current_command in "mMzZlLhHvVcCsSqQtT"

        # reset the previous ctrl2 point for smooth beziers
        if current_command not in 'cCsS':
            last_ctrl_point = None

        # parse all arguments
        # most likely, these will come in coordinate pairs of two (x and y)
        # but for horizontal and vertical lines they are individual x or y coordinates respectively
        arguments: List[float] = [float(line[n.start():n.end()]) for n in NUMBER.finditer(line)]

        # move to
        if current_command in 'mM':
            assert len(arguments) > 0 and len(arguments) % 2 == 0
            coordinates: List[V2f] = [V2f(x, y) for x, y in chunks(arguments, 2)]

            # finish current shape
            if current_shape.is_valid():
                result.append(Shape(current_shape))

            current_coordinate = coordinates[0] if current_command.isupper() else current_coordinate + coordinates[0]
            current_shape = ShapeBuilder(current_coordinate)
            for coord in coordinates[1:]:
                current_shape.add_segment(coord if current_command.isupper() else current_coordinate + coord)
            current_coordinate = coordinates[-1]

        # close path
        elif current_command in 'zZ':
            assert len(arguments) == 0
            current_coordinate = current_shape.close()
            if current_shape.is_valid():
                result.append(Shape(current_shape))
                current_shape = ShapeBuilder(current_coordinate)

        # line to
        elif current_command in 'lL':
            assert len(arguments) > 0 and len(arguments) % 2 == 0
            for coord in (V2f(x, y) for x, y in chunks(arguments, 2)):
                current_coordinate = coord if current_command.isupper() else current_coordinate + coord
                current_shape.add_segment(current_coordinate)

        # horizontal line
        elif current_command in 'hH':
            assert len(arguments) > 0
            for x in arguments:
                current_coordinate.x = x if current_command.isupper() else current_coordinate.x + x
                current_shape.add_segment(current_coordinate)

        # vertical line
        elif current_command in 'vV':
            assert len(arguments) > 0
            for y in arguments:
                current_coordinate.y = y if current_command.isupper() else current_coordinate.y + y
                current_shape.add_segment(current_coordinate)

        # cubic bezier
        elif current_command in 'cC':
            assert len(arguments) > 0 and len(arguments) % 6 == 0
            for a, b, p in ((V2f(x1, y1), V2f(x2, y2), V2f(x, y)) for x1, y1, x2, y2, x, y in chunks(arguments, 6)):
                ctrl1 = a if current_command.isupper() else current_coordinate + a
                last_ctrl_point = b if current_command.isupper() else current_coordinate + b
                current_coordinate = p if current_command.isupper() else current_coordinate + p
                current_shape.add_segment(current_coordinate, ctrl1, last_ctrl_point)

        # smooth cubic bezier
        elif current_command in 'sS':
            assert len(arguments) > 0 and len(arguments) % 4 == 0
            if last_ctrl_point is None:
                last_ctrl_point = current_coordinate
            for b, p in ((V2f(x2, y2), V2f(x, y)) for x2, y2, x, y in chunks(arguments, 4)):
                ctrl1 = current_coordinate + current_coordinate - last_ctrl_point
                last_ctrl_point = b if current_command.isupper() else current_coordinate + b
                current_coordinate = p if current_command.isupper() else current_coordinate + p
                current_shape.add_segment(current_coordinate, ctrl1, last_ctrl_point)

        # square bezier
        elif current_command in 'qQ':
            assert len(arguments) > 0 and len(arguments) % 4 == 0
            for a, p in ((V2f(x1, y1), V2f(x, y)) for x1, y1, x, y in chunks(arguments, 4)):
                start: V2f = current_coordinate
                last_ctrl_point = a if current_command.isupper() else current_coordinate + a
                current_coordinate = p if current_command.isupper() else current_coordinate + p
                ctrl2: V2f = last_ctrl_point * 2.
                current_shape.add_segment(current_coordinate, (start + ctrl2) / 3., (ctrl2 + current_coordinate) / 3.)

        # smooth square bezier
        elif current_command in 'tT':
            assert len(arguments) > 0 and len(arguments) % 2 == 0
            if last_ctrl_point is None:
                last_ctrl_point = current_coordinate
            for p in (V2f(x, y) for x, y in chunks(arguments, 2)):
                start: V2f = current_coordinate
                last_ctrl_point = current_coordinate + current_coordinate - last_ctrl_point
                current_coordinate = p if current_command.isupper() else current_coordinate + p
                ctrl2: V2f = last_ctrl_point * 2.
                current_shape.add_segment(current_coordinate, (start + ctrl2) / 3., (ctrl2 + current_coordinate) / 3.)

        current_command = None

    if current_shape.is_valid():
        result.append(Shape(current_shape))
    return result


xml_tree: Xml.ElementTree = Xml.parse('/home/clemens/code/notf/dev/img/notf_inline_white.svg')
path_element: Xml.Element
paths: List[PathDescription] = [
    PathDescription(name=path_element.attrib['id'], shapes=parse_shapes(path_element.attrib['d']))
    for path_element in xml_tree.getroot().findall('path', {'': SVG_NAMESPACE})
]

print(paths)
