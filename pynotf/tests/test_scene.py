import unittest

from pynotf.scene import Path, Scene, Widget, InputPlug, StateMachine
from pynotf.value import Value

from tests.utils import number_schema


########################################################################################################################


class TestPath(unittest.TestCase):
    def test_empty(self):
        self.assertTrue(Path().is_empty())
        self.assertTrue(Path('three/steps/down/../../..').is_empty())
        self.assertFalse(Path('/').is_empty())  # root
        self.assertFalse(Path('/test/this:path').is_empty())
        self.assertEqual(len(Path()), 0)
        self.assertEqual(len(Path('/')), 0)

    def test_absolute_paths(self):
        self.assertTrue(Path("/test/this:path").is_absolute())
        self.assertTrue(Path("/test/this").is_absolute())
        self.assertTrue(Path("/test/this/").is_absolute())
        self.assertTrue(Path("/test/this/../that").is_absolute())
        self.assertTrue(Path("/:path").is_absolute())
        self.assertFalse(Path("./test/this/../that").is_absolute())

    def test_relative_paths(self):
        self.assertTrue(Path("test/this:path").is_relative())
        self.assertTrue(Path("test/this").is_relative())
        self.assertTrue(Path("test/this/").is_relative())
        self.assertTrue(Path("./test/this/").is_relative())
        self.assertTrue(Path("./test/this:path").is_relative())
        self.assertTrue(Path("./test/this/../../that:path").is_relative())
        self.assertTrue(Path("./:path").is_relative())
        self.assertTrue(Path(":path").is_relative())
        self.assertTrue(Path(".").is_relative())
        self.assertFalse(Path("/test/this/../../that:path").is_relative())

    def test_is_widget_path(self):
        self.assertTrue(Path("this/is/a/valid/path").is_widget_path())
        self.assertTrue(Path("this/is/a/../valid/path").is_widget_path())
        self.assertTrue(Path("./this/is/a/../valid/path").is_widget_path())
        self.assertTrue(Path("/this/is/a/../valid/path").is_widget_path())
        self.assertTrue(Path("/").is_widget_path())
        self.assertTrue(Path(".").is_widget_path())
        self.assertFalse(Path("this/is/a/valid:path").is_widget_path())

    def test_is_property_path(self):
        self.assertTrue(Path("this/is/a/valid:path").is_element_path())
        self.assertTrue(Path("./test/this:path").is_element_path())
        self.assertTrue(Path("/test/this:path").is_element_path())
        self.assertTrue(Path("./:path").is_element_path())
        self.assertTrue(Path(":path").is_element_path())
        self.assertFalse(Path("this/is/a/valid:path").is_widget_path())

    def test_get_widget_path(self):
        self.assertEqual(Path("/test/that/../this:path").get_widget_path(), Path("/test/this"))
        self.assertEqual(Path("/test/that/../this").get_widget_path(), Path("/test/this"))
        self.assertEqual(Path("./test/that/../this:path").get_widget_path(), Path("./test/this"))
        self.assertEqual(Path("./test/that/../this").get_widget_path(), Path("./test/this"))

    def test_normalization(self):
        self.assertEqual(Path("/test/this:path"), Path("/test/that/../this:path"))
        self.assertEqual(Path("test/this:path"), Path("./test/that/../this:path"))
        self.assertEqual(Path("/test/this/"), Path("/test//this//"))
        self.assertEqual(Path("/test/./this/"), Path("/test/this/"))
        self.assertEqual(Path('two/down/../../../sibling/..'), Path('..'))
        self.assertEqual(Path('three/steps/down/../../../../../'), Path('../..'))

    def test_factory(self):
        self.assertEqual(Path.create(['test', 'this'], True, 'path'), Path("/test/this:path"))
        self.assertEqual(Path.create(['test', 'this'], element_name='path'), Path("/test/this:path"))
        self.assertEqual(Path.create(['test', 'this'], is_absolute=False), Path("test/this"))
        self.assertEqual(Path.create(['test', 'this']), Path("/test/this"))
        self.assertEqual(Path.create([], is_absolute=True), Path('/'))
        self.assertEqual(Path.create([], is_absolute=False), Path())

    def test_widget_following_property(self):
        with self.assertRaises(Path.Error):
            Path("test/node:property/wrong")

    def test_property_following_property(self):
        with self.assertRaises(Path.Error):
            Path("test/node:property:wrong")

    def test_empty_property(self):
        with self.assertRaises(Path.Error):
            Path("test/node:")

    def test_go_up_beyond_root(self):
        with self.assertRaises(Path.Error):
            Path("/root/child/../../../nope")

    def test_path_iteration(self):
        path: Path = Path('/parent/child/target:property')
        self.assertEqual(path.get_widget(0), 'parent')
        self.assertEqual(path.get_widget(1), 'child')
        self.assertEqual(path.get_widget(2), 'target')
        self.assertEqual(path.get_element(), 'property')
        self.assertIsNone(path.get_widget(3))
        self.assertIsNone(Path('/parent/child/target').get_element())

    def test_concatenation(self):
        self.assertEqual(Path('/parent') + Path('path/to/child'), Path('/parent/path/to/child'))
        self.assertEqual(Path('/parent') + Path('./path/to/child'), Path('/parent/path/to/child'))
        self.assertEqual(Path('/parent/child') + Path('../path/to/sibling'), Path('/parent/path/to/sibling'))
        self.assertEqual(Path('/parent') + Path('path/to/') + Path('./another') + Path('child'),
                         Path('/parent/path/to/another/child'))
        self.assertEqual(Path('/parent') + Path('path/to:property'), Path('/parent/path/to:property'))
        self.assertEqual(Path('/parent/child/target:property') + Path('../jup:prop'), Path('/parent/child/jup:prop'))
        self.assertEqual(Path('/') + Path('./test/this/../that'), Path('/test/that'))
        self.assertEqual(Path('what/is/this/') + Path('../that:stuff'), Path('what/is/that:stuff'))

    def test_concatenation_in_place(self):
        path: Path = Path('/parent')
        path += Path('path/to/child')
        self.assertEqual(path, Path('/parent/path/to/child'))

        path: Path = Path('/parent')
        path += Path('./path/to/child')
        self.assertEqual(path, Path('/parent/path/to/child'))

        path: Path = Path('/parent/child')
        path += Path('../path/to/sibling')
        self.assertEqual(path, Path('/parent/path/to/sibling'))

        path: Path = Path('/parent')
        path += Path('path/to/')
        path += Path('./another')
        path += Path('child')
        self.assertEqual(path, Path('/parent/path/to/another/child'))

        path: Path = Path('/parent')
        path += Path('path/to:property')
        self.assertEqual(path, Path('/parent/path/to:property'))

        path: Path = Path('/parent/child/target:property')
        path += Path('../jup:prop')
        self.assertEqual(path, Path('/parent/child/jup:prop'))

        path: Path = Path('/')
        path += Path('./test/this/../that')
        self.assertEqual(path, Path('/test/that'))

        path: Path = Path('what/is/this/')
        path += Path('../that:stuff')
        self.assertEqual(path, Path('what/is/that:stuff'))

    def test_concatenate_with_absolute(self):
        with self.assertRaises(Path.Error):
            Path('/parent/to/absolute') + Path('/another/absolute:property')
        with self.assertRaises(Path.Error):
            Path('path/to:property') + Path('/parent/to/absolute')
        with self.assertRaises(Path.Error):
            path: Path = Path('/parent/to/absolute')
            path += Path('/another/absolute:property')
        with self.assertRaises(Path.Error):
            path: Path = Path('path/to:property')
            path += Path('/parent/to/absolute')

    def test_repr(self):
        self.assertEqual(str(Path()), '')
        self.assertEqual(str(Path('/')), '/')
        self.assertEqual(str(Path('/test/this:path')), '/test/this:path')
        self.assertEqual(str(Path('./test/this/../that')), 'test/that')


########################################################################################################################

class TestEmptyScene(unittest.TestCase):

    def test_empty(self):
        _: Scene = Scene()

    def test_simple_setup(self):
        """                                  +------+
              '/'       with 'a' and 'b':   -| prop |-
               |                             |------|
            +--+--+                         -|input |
            |     |                          |output|-
           'a'   'b'                         +------+

        We have a simple setup with two Widgets of the same type under the root of the Scene.
        Each input of each Widget is connected to an outside Emitter for stimuli, the input of `b` is also connected
        to the output of `a`.
        """
        scene: Scene = Scene()
        scene.register_widget_type(Widget.Definition(
            type_name='test_widget',
            properties={
                'prop': Widget.PropertyDefinition(Value(100))
            },
            input_plugs={
                'input': number_schema
            },
            output_plugs={
                'output': number_schema
            },
            input_callbacks={
                'state1_input': InputPlug.Callback(r"""
                    number = signal.get_value().as_number()
                    if number <= 100:
                        widget['prop'] = number
                    else:
                        widget.emit('output', number % 100)
                        widget.transition_into('state2')
                    
                """),
                'state2_input': InputPlug.Callback(r"""
                    number = signal.get_value().as_number()
                    widget['prop'] = number * -1
                    if number == 0:
                        widget.transition_into('state1')
                """)
            },
            property_callbacks={},
            state_machine=StateMachine(StateMachine.Description(
                states={
                    'state1': StateMachine.State(StateMachine.Callback(r"""
                        widget.set_input_callback('input', 'state1_input')
                    """)),
                    'state2': StateMachine.State(StateMachine.Callback(r"""
                        widget.set_input_callback('input', 'state2_input')
                    """))
                },
                initial_state='state1',
                transitions={
                    'state1': ['state2'],
                    'state2': ['state1']
                }
            ))
        ))

        a: Widget = scene.create_widget('test_widget', 'a')
        b: Widget = scene.create_widget('test_widget', 'b')
