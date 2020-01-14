from __future__ import annotations
from typing import List, Optional


########################################################################################################################

class Path:
    """
    Every Path is immutable, and guaranteed consistent (albeit not necessarily valid) if the construction succeeded.
    """

    class Error(Exception):
        """
        Error thrown during the construction of a Path object.
        """
        @classmethod
        def show_location(cls, path: str, error_position: int, error_length: int, message: str) -> Path.Error:
            return Path.Error('Error when constructing a Path from a string:\n'
                              '\t"{0}"\n'
                              '\t{1:>{2}}\n'
                              '\t{3}'.format(path,
                                             '{0:^>{1}}'.format('^', error_length),
                                             error_position + error_length,
                                             message))

    @staticmethod
    def get_widget_delimiter() -> str:
        """
        Delimiter character used to separate Widgets in the Path.
        """
        return '/'

    @staticmethod
    def get_property_delimiter() -> str:
        """
        Delimiter character used to denote a final token in the Path.
        """
        return ':'
    
    @staticmethod
    def create(widgets: List[str], is_absolute: bool = True, property_name: Optional[str] = None):
        """
        Manual Path factory.
        :param widgets: All widgets in the Path.
        :param is_absolute: Whether or not the Path is absolute or not (default = True).
        :param property_name: Optional target Property of the last Widget in the Path (default = None).
        :return: A new Path.
        """
        path: Path = Path()
        path._widgets = widgets
        path._property = property_name
        path._is_absolute = is_absolute
        path._normalize()
        return path

    def __init__(self, string: str = ''):
        """
        Constructor.
        :param string: String from which to construct the Path.
        """
        # initialize member defaults
        self._widgets: List[str] = []
        self._property: Optional[str] = None
        self._is_absolute: bool = False

        # if the path is empty, this is all we have to do
        if len(string) == 0:
            return

        # absolute paths begin with the widget delimiter
        self._is_absolute = (string[0] == self.get_widget_delimiter())
        if self._is_absolute:
            string = string[1:]

            # return early if this path is the root path
            if len(string) == 0:
                return

        # test whether this is a path to a property
        property_delimiter_pos: int = string.find(self.get_property_delimiter())
        if property_delimiter_pos != -1:
            # empty property names are   not allowed
            if property_delimiter_pos == len(string) - 1:
                raise Path.Error.show_location(string, property_delimiter_pos + 1, 1,
                                               "Empty Property names are not allowed")

            # additional delimiters of any kind after the property delimiter are not allowed
            extra_delimiter_pos: int = max(string.find(self.get_widget_delimiter(), property_delimiter_pos + 1),
                                           string.find(self.get_property_delimiter(), property_delimiter_pos + 1))
            if extra_delimiter_pos != -1:
                raise Path.Error.show_location(string, extra_delimiter_pos + 1, len(string) - extra_delimiter_pos,
                                               "Delimiters in the Property name are not allowed")

        # split the path into widget tokens
        self._widgets: List[str] = string.split(self.get_widget_delimiter())

        # if this is a property path split the last token into widget:property
        if property_delimiter_pos != -1:
            last_widget, self._property = self._widgets[-1].split(self.get_property_delimiter())
            self._widgets[-1] = last_widget

        self._normalize()

    def _normalize(self):
        normalized_tokens: List[str] = []

        path: str = str(self)
        path_pos: int = 1 if self.is_absolute() else 0

        for token in self._widgets:
            token_pos: int = path_pos
            path_pos += len(token) + 1  # +1 for the delimiter

            # ignore empty tokens
            if len(token) == 0:
                continue

            # ignore "." in all but the special "self" path: "."
            if token == '.' and len(self._widgets) > 1:
                continue

            # depending on the path so far, ".." either removes the rightmost normalized token or is simply appended
            elif token == '..':
                if len(normalized_tokens) == 0:
                    if self._is_absolute:
                        raise Path.Error.show_location(path, token_pos, path_pos - token_pos,
                                                       'Absolute path cannot be resolved.')
                    else:
                        pass  # if the path is relative, store the ".." token at the beginning
                else:  # len(normalized_tokens) > 0
                    if normalized_tokens[-1] == '..':
                        assert not self._is_absolute
                        pass  # if the last token is already a ".." append the new ".."
                    else:
                        normalized_tokens.pop()  # if the token is a ".." and we know the parent node, go back one step
                        continue

            normalized_tokens.append(token)

        # replace the current tokens with the normalized ones
        self._widgets = normalized_tokens

    def is_empty(self) -> bool:
        """
        Checks whether the Path is empty.
        An absolute path without any Widgets is the root and not empty.
        """
        return len(self._widgets) == 0 and self._property is None and not self.is_absolute()

    def is_absolute(self) -> bool:
        """
        Tests if this Path is absolute or not.
        Absolute paths begin with a widget delimiter.
        """
        return self._is_absolute

    def is_relative(self) -> bool:
        """
        Tests if this path is relative (not absolute).
        """
        return not self._is_absolute

    def is_property_path(self) -> bool:
        """
        Checks whether or not the last token in the Path is a property name.
        """
        return self._property is not None

    def is_widget_path(self) -> bool:
        """
        Checks whether or not the last token in the Path is a widget name.
        """
        return self._property is None

    def get_widget(self, index: int) -> Optional[str]:
        """
        Returns the n'th Widget in the Path or None, if the index is out of bounds.
        :param index: Index of the requested Widget in the Path.
        """
        if index >= len(self._widgets):
            return None
        else:
            return self._widgets[index]

    def get_property(self) -> Optional[str]:
        """
        Returns the name of the Property pointed to by this Path or None if this is not a Property Path.
        """
        return self._property

    def __len__(self) -> int:
        """
        Number of Widgets in the normalized Path.
        """
        return len(self._widgets)

    def __repr__(self) -> str:
        """
        Build a string representation of this Path.
        """
        if self.is_empty():
            return ''

        elif self.is_property_path():
            return f'{self.get_widget_delimiter() if self._is_absolute else ""}' \
                   f'{self.get_widget_delimiter().join(self._widgets)}' \
                   f':{self._property}'
        else:
            return f'{self.get_widget_delimiter() if self._is_absolute else ""}' \
                   f'{self.get_widget_delimiter().join(self._widgets)}'

    def __eq__(self, other: Path):
        """
        Tests two Paths for equality.
        """
        return (self._is_absolute == other._is_absolute and
                self._widgets == other._widgets and
                self._property == other._property)

    def __add__(self, other: Path) -> Path:
        """
        Concatenate this Path with another one.
        :param other: Other Path to add.
        :return: New concatenated Path.
        :raise Path.Error: If other is an absolute Path.
        """
        if other.is_absolute():
            raise Path.Error(f'Cannot add path "{str(other)}" because it is absolute"')

        return Path.create(self._widgets + other._widgets, self.is_absolute(), other._property)

    def __iadd__(self, other: Path) -> Path:
        """
        In-place concatenation of this Path with other.
        :param other: Other Path to add.
        :return: This Path after other was concatenated to it.
        :raise Path.Error: If other is an absolute Path.
        """
        if other.is_absolute():
            raise Path.Error(f'Cannot add path "{str(other)}" because it is absolute"')

        self._widgets.extend(other._widgets)
        self._property = other._property
        self._normalize()
        return self
