import unittest

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
        self.assertFalse(empty_path.is_leaf_path())

    def test_invalid(self):
        """
        Non-empty paths can be invalid.
        """
        with self.assertRaises(Path.Error):
            Path("test/node:leaf/wrong")  # no nodes following a leaf
        with self.assertRaises(Path.Error):
            Path("test/node:leaf:wrong")  # no properties following a leaf
        with self.assertRaises(Path.Error):
            Path("test/wrong:")  # no empty leaf name
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
        absolute: Path = Path("/test/this:path")
        self.assertTrue(absolute.is_absolute())
        self.assertFalse(absolute.is_relative())
        self.assertFalse(absolute.is_empty())

        relative: Path = Path("test/this/path")
        self.assertTrue(relative.is_relative())
        self.assertFalse(relative.is_absolute())
        self.assertFalse(relative.is_empty())

    def test_node_leaf(self):
        """
        Paths can denote a node or a leaf.
        """
        node: Path = Path("/this/is/a/valid")
        self.assertTrue(node.is_node_path())
        self.assertFalse(node.is_leaf_path())
        self.assertEqual(node.get_node_path(), node)

        leaf: Path = Path("/this/is/a/valid:leaf")
        self.assertTrue(leaf.is_leaf_path())
        self.assertFalse(leaf.is_node_path())
        self.assertEqual(leaf.get_node_path(), node)
        self.assertEqual(leaf.get_leaf(), 'leaf')

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

    def test_relative_leaf(self):
        """
        Leafs can also be used in a relative path.
        """
        just_leaf: Path = Path(":leaf")
        self.assertTrue(just_leaf.is_relative())
        self.assertTrue(just_leaf.is_leaf_path())
        self.assertFalse(just_leaf.is_empty())

        node: Path = Path("/some/node")
        self.assertEqual(node + just_leaf, Path("/some/node:leaf"))

    def test_steps(self):
        """
        Paths can be iterated step-by-step.
        """
        path: Path = Path("/parent/child/node:leaf")
        self.assertEqual(len(path), 3)  # not counting the leaf
        self.assertEqual(path[0], 'parent')
        self.assertEqual(path[1], 'child')
        self.assertEqual(path[2], 'node')
        self.assertEqual(path[-1], 'node')
        self.assertEqual(path[-2], 'child')
        self.assertEqual(path[-3], 'parent')
        with self.assertRaises(IndexError):
            _ = path[3]
        with self.assertRaises(IndexError):
            _ = path[-4]

    def test_compare(self):
        """
        Paths can be compared.
        """
        self.assertTrue(Path("") == Path())
        self.assertTrue(Path("") != Path("."))  # left one is empty, right one is node
        self.assertTrue(Path("") != Path("/"))  # left one is relative, right one is absolute
        self.assertTrue(Path("node") != Path("/node"))
        self.assertTrue(Path("node") != Path(".node"))
        self.assertTrue(Path("leaf") != Path(":leaf"))
        self.assertTrue(Path("one") != Path("one/two"))
        self.assertTrue(Path("one/two") != Path("one/two/three"))
        self.assertTrue(Path("one/two") != Path("one/three"))
        self.assertTrue(Path("one/two") == Path("one/two"))
        self.assertTrue(Path("one/two:three") == Path("one/two:three"))

    def test_repr(self):
        """
        Paths can be converted back into a string.
        """
        self.assertEqual(str(Path()), "")
        self.assertEqual(str(Path("node")), "node")
        self.assertEqual(str(Path("/absolute/node")), "/absolute/node")
        self.assertEqual(str(Path("/absolute/node:leaf")), "/absolute/node:leaf")
        self.assertEqual(str(Path("relative/node:leaf")), "relative/node:leaf")
        self.assertEqual(str(Path("relative/../node:leaf")), "node:leaf")

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

        # ... down to a leaf
        self.assertEqual(Path("/parent") + Path("path/to:leaf"), Path("/parent/path/to:leaf"))

        # ... up from a leaf
        self.assertEqual(Path("/root/path/to/a:leaf") + Path("../another:leaf"), Path("/root/path/to/another:leaf"))

    def test_resolve_absolute(self):
        """
        Absolute paths cannot be concatenated...
        """
        # ... on the right of a relative path
        with self.assertRaises(Path.Error):
            Path("path/to:leaf") + Path("/parent/to/absolute")

        # ... to another another absolute path
        with self.assertRaises(Path.Error):
            Path("/parent/to/absolute") + Path("/another/absolute:leaf")

    def test_add_relative_to_leaf(self):
        """
        When appending a relative path to a leaf path, the leaf is ignored.
        """
        path: Path = Path("/parent/to/absolute:leaf") + Path("child/to/another:leaf")
        self.assertEqual(path, Path("/parent/to/absolute/child/to/another:leaf"))
        self.assertTrue(path.is_absolute())
        self.assertTrue(path.is_leaf_path())

        with self.assertRaises(Path.Error):
            _ = Path("/parent/to/absolute:leaf") + Path("/nope/does/not:work")

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
            Path.check_name('contains:colon')
        Path.check_name("this_should-work75")
        Path.check_name('name.with dot_and-others..')
