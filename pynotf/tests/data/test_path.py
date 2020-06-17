import unittest
from typing import List

from pynotf.data import Path


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def test_construction(self):
        """
        Construct and normalize various Paths.
        """
        self.assertEqual(str(Path()), '')
        self.assertEqual(str(Path('')), '')
        self.assertEqual(str(Path('.')), '')
        self.assertEqual(str(Path('..')), '..')
        self.assertEqual(str(Path('//')), '/')
        self.assertEqual(str(Path('./')), '')
        self.assertEqual(str(Path('../')), '..')
        self.assertEqual(str(Path('.//')), '')
        self.assertEqual(str(Path('/node')), '/node')
        self.assertEqual(str(Path('//node')), '/node')
        self.assertEqual(str(Path('./node')), 'node')
        self.assertEqual(str(Path('node/child')), 'node/child')
        self.assertEqual(str(Path('node//child')), 'node/child')
        self.assertEqual(str(Path('node|interop')), 'node|interop')
        self.assertEqual(str(Path('/node|interop')), '/node|interop')
        self.assertEqual(str(Path('|interop')), '|interop')
        self.assertEqual(str(Path('.|interop')), '|interop')
        self.assertEqual(str(Path('./|interop')), '|interop')
        self.assertEqual(str(Path('../|interop')), '..|interop')
        self.assertEqual(str(Path('../.')), '..')
        self.assertEqual(str(Path('../..')), '../..')
        self.assertEqual(str(Path('../../down/')), '../../down')
        self.assertEqual(str(Path('three/steps/down/../../..')), '')
        self.assertEqual(str(Path('three/steps/down/../../../../../')), '../..')
        self.assertEqual(str(Path('two/down/../../../sibling/..')), '..')
        self.assertEqual(str(Path('/parent/./child/../child/target/')), '/parent/child/target')
        self.assertEqual(str(Path('parent/./child/../child/target/|interop')), 'parent/child/target|interop')
        self.assertEqual(str(Path('service:/|/://.||::')), 'service:/|/://.||::')

    def test_construction_failure(self):
        """
        Fail to construct various invalid Paths.
        """
        with self.assertRaises(Path.Error):
            Path('test/node|interop/wrong')  # no nodes following a interop
        with self.assertRaises(Path.Error):
            Path('test/node|interop|wrong')  # a interop cannot follow another interop
        with self.assertRaises(Path.Error):
            Path('test/wrong|')  # empty interop name
        with self.assertRaises(Path.Error):
            Path('/root/child/../../../nope')  # going up further than the root in absolute paths
        with self.assertRaises(Path.Error):
            Path('/node:service')  # service delimiter must be the first if present
        with self.assertRaises(Path.Error):
            Path(':')  # empty service

    def test_assembly(self):
        """
        A Node or Interop-Path can be assembled from pieces.
        """
        self.assertEqual(Path.assemble(["parent", "child"]), Path("/parent/child"))
        self.assertEqual(Path.assemble(["parent", "child"], interop='interop'), Path("/parent/child|interop"))
        self.assertEqual(Path.assemble(["parent", "child"], is_relative=True), Path("parent/child"))
        self.assertEqual(Path.assemble(["parent", "child"], interop='interop', is_relative=True),
                         Path("parent/child|interop"))
        self.assertEqual(Path.assemble(["parent", "..", "child"]), Path("/child"))

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
        Path.check_name('this_should-work75')
        Path.check_name('name.with dot_and-others..')

    def test_is_empty(self):
        """
        Check that an empty Path is always recognized.
        """
        self.assertTrue(Path().is_empty())
        self.assertFalse(Path('/').is_empty())
        self.assertFalse(Path('/|interop').is_empty())
        self.assertFalse(Path('|interop').is_empty())
        self.assertFalse(Path('..').is_empty())
        self.assertFalse(Path('..|interop').is_empty())
        self.assertFalse(Path('simple').is_empty())
        self.assertFalse(Path('/parent/child').is_empty())
        self.assertFalse(Path('/parent/child|interop').is_empty())
        self.assertFalse(Path('parent/child').is_empty())
        self.assertFalse(Path('parent/child|interop').is_empty())
        self.assertFalse(Path('../parent/child').is_empty())
        self.assertFalse(Path('../parent/child|interop').is_empty())
        self.assertFalse(Path('service:path').is_empty())

    def test_is_absolute(self):
        """
        Check that absolute Paths are always recognized.
        """
        self.assertFalse(Path().is_absolute())
        self.assertTrue(Path('/').is_absolute())
        self.assertTrue(Path('/|interop').is_absolute())
        self.assertFalse(Path('|interop').is_absolute())
        self.assertFalse(Path('..').is_absolute())
        self.assertFalse(Path('..|interop').is_absolute())
        self.assertFalse(Path('simple').is_absolute())
        self.assertTrue(Path('/parent/child').is_absolute())
        self.assertTrue(Path('/parent/child|interop').is_absolute())
        self.assertFalse(Path('parent/child').is_absolute())
        self.assertFalse(Path('parent/child|interop').is_absolute())
        self.assertFalse(Path('../parent/child').is_absolute())
        self.assertFalse(Path('../parent/child|interop').is_absolute())
        self.assertTrue(Path('service:path').is_absolute())

    def test_is_relative(self):
        """
        Check that relative Paths are always recognized.
        """
        self.assertTrue(Path().is_relative())
        self.assertFalse(Path('/').is_relative())
        self.assertFalse(Path('/|interop').is_relative())
        self.assertTrue(Path('|interop').is_relative())
        self.assertTrue(Path('..').is_relative())
        self.assertTrue(Path('..|interop').is_relative())
        self.assertTrue(Path('simple').is_relative())
        self.assertFalse(Path('/parent/child').is_relative())
        self.assertFalse(Path('/parent/child|interop').is_relative())
        self.assertTrue(Path('parent/child').is_relative())
        self.assertTrue(Path('parent/child|interop').is_relative())
        self.assertTrue(Path('../parent/child').is_relative())
        self.assertTrue(Path('../parent/child|interop').is_relative())
        self.assertFalse(Path('service:path').is_relative())

    def test_is_node_path(self):
        """
        Check that Node Paths are always recognized.
        """
        self.assertTrue(Path().is_node_path())
        self.assertTrue(Path('/').is_node_path())
        self.assertFalse(Path('/|interop').is_node_path())
        self.assertFalse(Path('|interop').is_node_path())
        self.assertTrue(Path('..').is_node_path())
        self.assertFalse(Path('..|interop').is_node_path())
        self.assertTrue(Path('simple').is_node_path())
        self.assertTrue(Path('/parent/child').is_node_path())
        self.assertFalse(Path('/parent/child|interop').is_node_path())
        self.assertTrue(Path('parent/child').is_node_path())
        self.assertFalse(Path('parent/child|interop').is_node_path())
        self.assertTrue(Path('../parent/child').is_node_path())
        self.assertFalse(Path('../parent/child|interop').is_node_path())
        self.assertFalse(Path('service:path').is_node_path())

    def test_is_interop_path(self):
        """
        Check that Interop Paths are always recognized.
        """
        self.assertFalse(Path().is_interop_path())
        self.assertFalse(Path('/').is_interop_path())
        self.assertTrue(Path('/|interop').is_interop_path())
        self.assertTrue(Path('|interop').is_interop_path())
        self.assertFalse(Path('..').is_interop_path())
        self.assertTrue(Path('..|interop').is_interop_path())
        self.assertFalse(Path('simple').is_interop_path())
        self.assertFalse(Path('/parent/child').is_interop_path())
        self.assertTrue(Path('/parent/child|interop').is_interop_path())
        self.assertFalse(Path('parent/child').is_interop_path())
        self.assertTrue(Path('parent/child|interop').is_interop_path())
        self.assertFalse(Path('../parent/child').is_interop_path())
        self.assertTrue(Path('../parent/child|interop').is_interop_path())
        self.assertFalse(Path('service:path').is_interop_path())

    def test_is_service(self):
        """
        Check that Service Paths are always recognized.
        """
        self.assertFalse(Path().is_service_path())
        self.assertFalse(Path('/').is_service_path())
        self.assertFalse(Path('/|interop').is_service_path())
        self.assertFalse(Path('|interop').is_service_path())
        self.assertFalse(Path('..').is_service_path())
        self.assertFalse(Path('..|interop').is_service_path())
        self.assertFalse(Path('simple').is_service_path())
        self.assertFalse(Path('/parent/child').is_service_path())
        self.assertFalse(Path('/parent/child|interop').is_service_path())
        self.assertFalse(Path('parent/child').is_service_path())
        self.assertFalse(Path('parent/child|interop').is_service_path())
        self.assertFalse(Path('../parent/child').is_service_path())
        self.assertFalse(Path('../parent/child|interop').is_service_path())
        self.assertTrue(Path('service:path').is_service_path())

    def test_is_simple(self):
        """
        Simple Paths are a single, non-empty name.
        """
        self.assertFalse(Path().is_simple())
        self.assertFalse(Path('/').is_simple())
        self.assertFalse(Path('/|interop').is_simple())
        self.assertFalse(Path('|interop').is_simple())
        self.assertFalse(Path('..').is_simple())
        self.assertFalse(Path('..|interop').is_simple())
        self.assertTrue(Path('simple').is_simple())
        self.assertFalse(Path('/parent/child').is_simple())
        self.assertFalse(Path('/parent/child|interop').is_simple())
        self.assertFalse(Path('parent/child').is_simple())
        self.assertFalse(Path('parent/child|interop').is_simple())
        self.assertFalse(Path('../parent/child').is_simple())
        self.assertFalse(Path('../parent/child|interop').is_simple())
        self.assertFalse(Path('service:path').is_simple())

    def test_get_node_path(self):
        """
        Node and Interop Paths can be asked for only the Node Path part.
        """
        self.assertEqual(Path().get_node_path(), Path())
        self.assertEqual(Path('/').get_node_path(), Path('/'))
        self.assertEqual(Path('/|interop').get_node_path(), Path('/'))
        self.assertEqual(Path('|interop').get_node_path(), Path())
        self.assertEqual(Path('..').get_node_path(), Path('..'))
        self.assertEqual(Path('..|interop').get_node_path(), Path('..'))
        self.assertEqual(Path('simple').get_node_path(), Path('simple'))
        self.assertEqual(Path('/parent/child').get_node_path(), Path('/parent/child'))
        self.assertEqual(Path('/parent/child|interop').get_node_path(), Path('/parent/child'))
        self.assertEqual(Path('parent/child').get_node_path(), Path('parent/child'))
        self.assertEqual(Path('parent/child|interop').get_node_path(), Path('parent/child'))
        self.assertIsNone(Path('service:path').get_node_path())

    def test_get_interop_name(self):
        """
        Node and Interop Paths can be asked for only the Node Path part.
        """
        self.assertIsNone(Path().get_interop_name())
        self.assertIsNone(Path('/').get_interop_name())
        self.assertEqual(Path('/|interop').get_interop_name(), 'interop')
        self.assertEqual(Path('|interop').get_interop_name(), 'interop')
        self.assertIsNone(Path('..').get_interop_name())
        self.assertEqual(Path('..|interop').get_interop_name(), 'interop')
        self.assertIsNone(Path('simple').get_interop_name())
        self.assertIsNone(Path('/parent/child').get_interop_name())
        self.assertEqual(Path('/parent/child|interop').get_interop_name(), 'interop')
        self.assertIsNone(Path('parent/child').get_interop_name())
        self.assertEqual(Path('parent/child|interop').get_interop_name(), 'interop')
        self.assertIsNone(Path('service:path').get_interop_name())

    def test_path_iteration(self):
        """
        Node Paths can be iterated Node-by-Node.
        """
        path: Path = Path('/parent/child/node|interop')
        nodes: List[str] = [name for name in path.get_iterator()]
        self.assertEqual(len(nodes), 3)
        self.assertEqual(nodes[0], 'parent')
        self.assertEqual(nodes[1], 'child')
        self.assertEqual(nodes[2], 'node')

        path: Path = Path('parent/child')
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 2)
        self.assertEqual(nodes[0], 'parent')
        self.assertEqual(nodes[1], 'child')

        path: Path = Path('single_node')
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 1)
        self.assertEqual(nodes[0], 'single_node')

        path: Path = Path('/|some_fact')
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

        path: Path = Path('|relative_interop')
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

        path: Path = Path('/')
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

        path: Path = Path()
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

        path: Path = Path('service:path')
        nodes: List[str] = [name for name in path]
        self.assertEqual(len(nodes), 0)

    def test_path_length(self):
        """
        The length of a Path is the number of nodes in it.
        """
        self.assertEqual(len(Path()), 0)
        self.assertEqual(len(Path('/')), 1)
        self.assertEqual(len(Path('/|interop')), 1)
        self.assertEqual(len(Path('|interop')), 0)
        self.assertEqual(len(Path('..')), 1)
        self.assertEqual(len(Path('..|interop')), 1)
        self.assertEqual(len(Path('/parent/child')), 2)
        self.assertEqual(len(Path('/parent/child|interop')), 2)
        self.assertEqual(len(Path('parent/child')), 2)
        self.assertEqual(len(Path('parent/child|interop')), 2)
        self.assertEqual(len(Path('../parent/child')), 3)
        self.assertEqual(len(Path('../parent/child|interop')), 3)
        self.assertEqual(len(Path('service:path')), 0)

    def test_path_concatenation(self):
        """
        Paths can be concatenated.
        """
        self.assertEqual(Path('/some/node') + Path('|interop'), Path('/some/node|interop'))

        #
        # Relative paths can be resolved to absolute paths...

        # ... with an empty absolute path
        self.assertEqual(Path('/') + Path('path/to/child'), Path('/path/to/child'))

        # ... by going down in the hierarchy
        self.assertEqual(Path('/parent') + Path('path/to/child'), Path('/parent/path/to/child'))

        # ... by up down in the hierarchy
        self.assertEqual(Path('/parent/child') + Path('../path/to/sibling'), Path('/parent/path/to/sibling'))

        # ... by explicitly referring to the current
        self.assertEqual(Path('/parent') + Path('./path/to/child'), Path('/parent/path/to/child'))

        # ... by explicitly referring to the current
        self.assertEqual(Path('/parent') + Path('./path/to/child'), Path('/parent/path/to/child'))

        # ... by concatenating multiple relative paths
        self.assertEqual(Path('/parent') + Path('path/to/') + Path('./another') + Path('child'),
                         Path('/parent/path/to/another/child'))

        # ... down to an interop
        self.assertEqual(Path('/parent') + Path('path/to|interop'), Path('/parent/path/to|interop'))

        # ... up from an interop
        self.assertEqual(Path('/root/path/to/a|interop') + Path('../another|interop'),
                         Path('/root/path/to/another|interop'))

        #
        # Absolute paths cannot be concatenated...

        # ... on the right of a relative path
        with self.assertRaises(Path.Error):
            Path('path/to|interop') + Path('/parent/to/absolute')

        # ... to another another absolute path
        with self.assertRaises(Path.Error):
            Path('/parent/to/absolute') + Path('/another/absolute|interop')

        #
        # Service paths can also not be concatenated

        # ... on the left
        with self.assertRaises(Path.Error):
            Path('service:path') + Path('relative/with|interop')

        # ... or the right
        with self.assertRaises(Path.Error):
            Path('/parent/to/absolute') + Path('service:path')

        #
        # When appending a relative path to an interop path, the left interop is ignored.
        self.assertEqual(Path('/parent/to/absolute|interop') + Path('child/to/another|interop'),
                         Path('/parent/to/absolute/child/to/another|interop'))

    def test_in_place_concatenation(self):
        """
        Paths can be modified in place.
        """
        path: Path = Path('/parent')
        path += Path('path/to/')
        path += Path('./another')
        path += Path('child')
        self.assertEqual(path, Path('/parent/path/to/another/child'))


if __name__ == '__main__':
    unittest.main()
