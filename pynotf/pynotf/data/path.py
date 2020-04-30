from __future__ import annotations

from typing import List, Optional

NODE_DELIMITER: str = '/'
LEAF_DELIMITER: str = ':'
STEP_IN_PLACE: str = '.'
STEP_UP: str = '..'


########################################################################################################################

class Path:
    """
    Every Path is immutable, and guaranteed consistent (albeit not necessarily valid) if the construction succeeded.
    """

    STEP_IN_PLACE: str = STEP_IN_PLACE
    STEP_UP: str = STEP_UP

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
    def create(nodes: List[str], is_absolute: bool = True, leaf: Optional[str] = None) -> Path:
        """
        Manual Path factory.
        :param nodes: All Nodes in the Path.
        :param is_absolute: Whether or not the Path is absolute or not (default = True).
        :param leaf: Optional Leaf of the last Node in the Path (default = None).
        :return: A new Path.
        """
        path: Path = Path()
        path._nodes = nodes
        path._leaf = leaf
        path._is_absolute = is_absolute
        path._normalize()
        return path

    @staticmethod
    def check_name(name: str):
        """
        A valid Name identifier in notf must not be empty and must not control any Path control characters.
        """
        if not isinstance(name, str):
            raise NameError('Names must be strings')
        if name == "":
            raise NameError('Names may not be empty')
        if name == STEP_IN_PLACE or name == STEP_UP:
            raise NameError(f'The name "{name}" is reserved.')
        for path_control_character in ('/', ':'):
            if path_control_character in name:
                raise NameError('Names may not contain Path control characters slash [/] and colon [:]')

    def __init__(self, string: str = ''):
        """
        Constructor.
        :param string: String from which to construct the Path.
        """
        # initialize member defaults
        self._nodes: List[str] = []
        self._leaf: Optional[str] = None
        self._is_absolute: bool = False

        # if the path is empty, this is all we have to do
        if len(string) == 0:
            return

        # absolute paths begin with the node delimiter
        self._is_absolute = (string[0] == NODE_DELIMITER)
        if self._is_absolute:
            string = string[1:]

            # return early if this path is the root path
            if len(string) == 0:
                return

        # test whether this is a path to a leaf
        leaf_delimiter_pos: int = string.find(LEAF_DELIMITER)
        if leaf_delimiter_pos != -1:
            # empty leaf names are not allowed
            if leaf_delimiter_pos == len(string) - 1:
                raise Path.Error.show_location(string, leaf_delimiter_pos + 1, 1, "Empty Leaf names are not allowed")

            # additional delimiters of any kind after the leaf delimiter are not allowed
            extra_delimiter_pos: int = max(string.find(NODE_DELIMITER, leaf_delimiter_pos + 1),
                                           string.find(LEAF_DELIMITER, leaf_delimiter_pos + 1))
            if extra_delimiter_pos != -1:
                raise Path.Error.show_location(string, extra_delimiter_pos + 1, len(string) - extra_delimiter_pos,
                                               "Delimiters in the Leaf name are not allowed")

        # split the path into node tokens
        self._nodes: List[str] = string.split(NODE_DELIMITER)

        # if this is a leaf path split the last token into node:leaf
        if leaf_delimiter_pos != -1:
            last_node, self._leaf = self._nodes[-1].split(LEAF_DELIMITER)
            self._nodes[-1] = last_node

        self._normalize()

    def _normalize(self):
        normalized_tokens: List[str] = []

        path: str = str(self)
        path_pos: int = 1 if self.is_absolute() else 0

        for token in self._nodes:
            token_pos: int = path_pos
            path_pos += len(token) + 1  # +1 for the delimiter

            # ignore empty tokens
            if len(token) == 0:
                continue

            # ignore "." in all but the special "self" path: "."
            if token == STEP_IN_PLACE and len(self._nodes) > 1:
                continue

            # depending on the path so far, ".." either removes the rightmost normalized token or is simply appended
            elif token == STEP_UP:
                if len(normalized_tokens) == 0:
                    if self._is_absolute:
                        raise Path.Error.show_location(
                            path, token_pos, path_pos - token_pos, 'Absolute Path cannot be resolved.')
                    else:
                        pass  # if the path is relative, store the ".." token at the beginning
                else:  # len(normalized_tokens) > 0
                    if normalized_tokens[-1] == STEP_UP:
                        assert not self._is_absolute
                        pass  # if the last token is already a ".." append the new ".."
                    else:
                        normalized_tokens.pop()  # if the token is a ".." and we know the parent node, go back one step
                        continue

            normalized_tokens.append(token)

        # replace the current tokens with the normalized ones
        self._nodes = normalized_tokens

    def is_empty(self) -> bool:
        """
        Checks whether the Path is empty.
        An absolute path without any Nodes is the root and not empty.
        """
        return len(self._nodes) == 0 and self._leaf is None and not self.is_absolute()

    def is_absolute(self) -> bool:
        """
        Tests if this Path is absolute or not.
        Absolute paths begin with a Node delimiter.
        """
        return self._is_absolute

    def is_relative(self) -> bool:
        """
        Tests if this path is relative (not absolute).
        """
        return not self._is_absolute

    def is_leaf_path(self) -> bool:
        """
        Checks whether or not the last token in the Path is a Leaf.
        """
        return self._leaf is not None

    def is_node_path(self) -> bool:
        """
        Checks whether or not the last token in the Path is a Node.
        """
        return self._leaf is None and len(self._nodes) != 0

    def get_node_path(self) -> Path:
        """
        Returns this Path as a Node Path (without a Leaf postfix).
        If this is already a Node Path, the returned one is just a copy of this.
        """
        return self.create(self._nodes, self._is_absolute)

    def get_leaf(self) -> Optional[str]:
        """
        Returns the name of the Leaf pointed to by this Path or None if this is not an Leaf Path.
        """
        return self._leaf

    def __len__(self) -> int:
        """
        Number of Nodes in the normalized Path.
        """
        return len(self._nodes)

    def __repr__(self) -> str:
        """
        Build a string representation of this Path.
        """
        if self.is_empty():
            return ''

        elif self.is_leaf_path():
            return f'{NODE_DELIMITER if self._is_absolute else ""}' \
                   f'{NODE_DELIMITER.join(self._nodes)}' \
                   f':{self._leaf}'
        else:
            return f'{NODE_DELIMITER if self._is_absolute else ""}' \
                   f'{NODE_DELIMITER.join(self._nodes)}'

    def __eq__(self, other: Path):
        """
        Tests two Paths for equality.
        """
        return (self._is_absolute == other._is_absolute and
                self._nodes == other._nodes and
                self._leaf == other._leaf)

    def __getitem__(self, index: int) -> Optional[str]:
        """
        Returns the n'th Node in the Path or None, if the index is out of bounds.
        :param index: Index of the requested Node in the Path.
        :raise: IndexError if index >= number of Nodes in the Path.
        """
        if index < len(self._nodes):
            return self._nodes[index]
        else:
            raise IndexError(f'Cannot access Node {index} as Path {self!r} only contains {len(self)} nodes')

    def __add__(self, other: Path) -> Path:
        """
        Concatenate this Path with another one.
        :param other: Other Path to add.
        :return: New concatenated Path.
        :raise Path.Error: If other is an absolute Path.
        """
        if other.is_absolute():
            raise Path.Error(f'Cannot add Path "{str(other)}" because it is absolute"')

        return Path.create(self._nodes + other._nodes, self.is_absolute(), other._leaf)

    def __iadd__(self, other: Path) -> Path:
        """
        In-place concatenation of this Path with other.
        :param other: Other Path to add.
        :return: This Path after other was concatenated to it.
        :raise Path.Error: If other is an absolute Path.
        """
        if other.is_absolute():
            raise Path.Error(f'Cannot add Path "{str(other)}" because it is absolute"')

        self._nodes.extend(other._nodes)
        self._leaf = other._leaf
        self._normalize()
        return self
