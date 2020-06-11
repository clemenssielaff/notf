import unittest
from typing import List

from pynotf.data import Path


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def test_empty(self):
        """
        Default constructed paths are empty but valid and relative.
        """
        empty_path: Path = Path()
        self.assertTrue(empty_path.is_empty())
        self.assertTrue(empty_path.is_relative())
        self.assertFalse(empty_path.is_absolute())
        self.assertFalse(empty_path.is_node_path())
        self.assertFalse(empty_path.is_interop_path())

    def test_invalid(self):
        """
        Non-empty paths can be invalid.
        """
        with self.assertRaises(Path.Error):
            Path("test/node|interop/wrong")  # no nodes following a interop
        with self.assertRaises(Path.Error):
            Path("test/node|interop|wrong")  # a interop cannot follow another interop
        with self.assertRaises(Path.Error):
            Path("test/wrong|")  # no empty interop name
        with self.assertRaises(Path.Error):
            Path("/root/child/../../../nope")  # no going up further than the root in absolute paths

    def test_ignore_superfluous_symbols(self):
        """
        Superfluous symbols are ignored.
        """
        path: Path = Path("/parent/./child/../child/target/")
        self.assertEqual(path, Path("/parent/child/target"))
        self.assertEqual(len(path), 3)

    def test_absolute_relative(self):
        """
        Paths can be absolute or relative.
        """
        absolute: Path = Path("/test/this|path")
        self.assertTrue(absolute.is_absolute())
        self.assertFalse(absolute.is_relative())
        self.assertFalse(absolute.is_empty())

        relative: Path = Path("test/this/path")
        self.assertTrue(relative.is_relative())
        self.assertFalse(relative.is_absolute())
        self.assertFalse(relative.is_empty())

    def test_interop(self):
        """
        Paths can denote a node or an interop.
        """
        node: Path = Path("/this/is/a/valid")
        self.assertTrue(node.is_node_path())
        self.assertFalse(node.is_interop_path())
        self.assertEqual(node.get_node_path(), node)

        interop: Path = Path("/this/is/a/valid|interop")
        self.assertTrue(interop.is_interop_path())
        self.assertFalse(interop.is_node_path())
        self.assertEqual(interop.get_node_path(), node)
        self.assertEqual(interop.get_interop(), 'interop')

    def test_step_in_place(self):
        """
        A special relative path consists only of a single dot.
        """
        in_place: Path = Path(".")
        self.assertTrue(in_place.is_node_path())
        self.assertTrue(in_place.is_relative())

    def test_step_up(self):
        """
        Paths can contain '..'-tokens.
        """
        empty: Path = Path("three/steps/down/../../..")
        self.assertTrue(empty.is_empty())

        one_up: Path = Path("two/down/../../../sibling/..")
        self.assertEqual(one_up, Path(".."))
        self.assertTrue(one_up.is_relative())
        self.assertTrue(one_up.is_node_path())
        self.assertFalse(one_up.is_empty())

        two_up: Path = Path("three/steps/down/../../../../../")
        self.assertEqual(two_up, Path("../.."))
        self.assertTrue(two_up.is_relative())
        self.assertTrue(two_up.is_node_path())
        self.assertFalse(two_up.is_empty())

        one_down: Path = Path("../../down/")
        self.assertEqual(one_down, Path("../../down"))
        self.assertTrue(one_down.is_relative())
        self.assertTrue(one_down.is_node_path())

        up_and_down: Path = Path("/root/one/up/../down")
        self.assertEqual(up_and_down, Path("/root/one/down"))
        self.assertTrue(up_and_down.is_absolute())
        self.assertTrue(up_and_down.is_node_path())

    def test_relative_interop(self):
        """
        Interop can also be used in a relative path.
        """
        just_interop: Path = Path("|interop")
        self.assertTrue(just_interop.is_relative())
        self.assertTrue(just_interop.is_interop_path())
        self.assertFalse(just_interop.is_empty())

        node: Path = Path("/some/node")
        self.assertEqual(node + just_interop, Path("/some/node|interop"))

    def test_steps(self):
        """
        Paths can be iterated step-by-step.
        """
        path: Path = Path("/parent/child/node|interop")
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 3)
        self.assertEqual(nodes[0], 'parent')
        self.assertEqual(nodes[1], 'child')
        self.assertEqual(nodes[2], 'node')

        path: Path = Path("relative_node")
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 1)
        self.assertEqual(nodes[0], 'relative_node')

        path: Path = Path("/|some_fact")
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

        path: Path = Path("|relative_interop")
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

        path: Path = Path("/")
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

    def test_compare(self):
        """
        Paths can be compared.
        """
        self.assertTrue(Path("") == Path())
        self.assertTrue(Path("") != Path("."))  # left one is empty, right one is node
        self.assertTrue(Path("") != Path("/"))  # left one is relative, right one is absolute
        self.assertTrue(Path("node") != Path("/node"))
        self.assertTrue(Path("node") != Path(".node"))
        self.assertTrue(Path("interop") != Path("|interop"))
        self.assertTrue(Path("one") != Path("one/two"))
        self.assertTrue(Path("one/two") != Path("one/two/three"))
        self.assertTrue(Path("one/two") != Path("one/three"))
        self.assertTrue(Path("one/two") == Path("one/two"))
        self.assertTrue(Path("one/two|three") == Path("one/two|three"))

    def test_repr(self):
        """
        Paths can be converted back into a string.
        """
        self.assertEqual(str(Path()), "")
        self.assertEqual(str(Path("node")), "node")
        self.assertEqual(str(Path("/absolute/node")), "/absolute/node")
        self.assertEqual(str(Path("/absolute/node|interop")), "/absolute/node|interop")
        self.assertEqual(str(Path("relative/node|interop")), "relative/node|interop")
        self.assertEqual(str(Path("relative/../node|interop")), "node|interop")

    def test_resolve_relative(self):
        """
        Relative paths can be resolved to absolute paths...
        """
        # ... with an empty absolute path
        self.assertEqual(Path("/") + Path("path/to/child"), Path("/path/to/child"))

        # ... by going down in the hierarchy
        self.assertEqual(Path("/parent") + Path("path/to/child"), Path("/parent/path/to/child"))

        # ... by up down in the hierarchy
        self.assertEqual(Path("/parent/child") + Path("../path/to/sibling"), Path("/parent/path/to/sibling"))

        # ... by explicitly referring to the current
        self.assertEqual(Path("/parent") + Path("./path/to/child"), Path("/parent/path/to/child"))

        # ... by explicitly referring to the current
        self.assertEqual(Path("/parent") + Path("./path/to/child"), Path("/parent/path/to/child"))

        # ... by concatenating multiple relative paths
        self.assertEqual(Path("/parent") + Path("path/to/") + Path("./another") + Path("child"),
                         Path("/parent/path/to/another/child"))

        # ... down to an interop
        self.assertEqual(Path("/parent") + Path("path/to|interop"), Path("/parent/path/to|interop"))

        # ... up from an interop
        self.assertEqual(Path("/root/path/to/a|interop") + Path("../another|interop"),
                         Path("/root/path/to/another|interop"))

    def test_resolve_absolute(self):
        """
        Absolute paths cannot be concatenated...
        """
        # ... on the right of a relative path
        with self.assertRaises(Path.Error):
            Path("path/to|interop") + Path("/parent/to/absolute")

        # ... to another another absolute path
        with self.assertRaises(Path.Error):
            Path("/parent/to/absolute") + Path("/another/absolute|interop")

    def test_add_relative_to_interop(self):
        """
        When appending a relative path to an interop path, the interop is ignored.
        """
        path: Path = Path("/parent/to/absolute|interop") + Path("child/to/another|interop")
        self.assertEqual(path, Path("/parent/to/absolute/child/to/another|interop"))
        self.assertTrue(path.is_absolute())
        self.assertTrue(path.is_interop_path())

        with self.assertRaises(Path.Error):
            _ = Path("/parent/to/absolute|interop") + Path("/nope/does/not|work")

    def test_add_in_place(self):
        """
        Paths can be modified in place.
        """
        path: Path = Path("/parent")
        path += Path("path/to/")
        path += Path("./another")
        path += Path("child")
        self.assertEqual(path, Path("/parent/path/to/another/child"))

        with self.assertRaises(Path.Error):
            path += Path("/won't/work/because/absolute")

    def test_check_name(self):
        """
        Names in the path must not contain control characters.
        """
        with self.assertRaises(NameError):
            # noinspection PyTypeChecker
            Path.check_name(None)  # not a string
        with self.assertRaises(NameError):
            Path.check_name('')  # empty
        with self.assertRaises(NameError):
            Path.check_name('.')  # reserved
        with self.assertRaises(NameError):
            Path.check_name('..')  # reserved
        with self.assertRaises(NameError):
            Path.check_name('contains/slash')
        with self.assertRaises(NameError):
            Path.check_name('contains|pipe')
        Path.check_name("this_should-work75")
        Path.check_name('name.with dot_and-others..')
