from __future__ import annotations

from typing import List, Optional, Iterator

SERVICE_DELIMITER: str = ':'
NODE_DELIMITER: str = '/'
INTEROP_DELIMITER: str = '|'
DELIMITERS: str = f'{SERVICE_DELIMITER}{NODE_DELIMITER}{INTEROP_DELIMITER}'
STEP_IN_PLACE: str = '.'
STEP_UP: str = '..'


# PATH ERROR ###########################################################################################################

class PathError(Exception):
    """
    Error thrown during the construction of a Path object.
    """

    @classmethod
    def show_location(cls, path: str, error_position: int, error_length: int, message: str) -> Path.Error:
        return Path.Error('Error when constructing a Path from string:\n'
                          '\t"{0}"\n'
                          '\t{1:>{2}}\n'
                          '\t{3}'.format(path,
                                         '{0:^>{1}}'.format('^', error_length),
                                         error_position + 1 + error_length,
                                         message))


# PATH #################################################################################################################

class Path:
    """
    Paths identify a Node or Interop in the Scene, or a Fact provided by a Service.
    Internally, they are simply an immutable string, with all supplementary information encoded within it.
    All Paths are guaranteed to be valid if the construction succeeded.
    """

    Error = PathError
    STEP_UP = STEP_UP

    def __init__(self, string: str = ''):
        """
        Constructor.
        :param string: String from which to construct the Path.
        """
        self._string: str = string
        self._normalize()

    @staticmethod
    def _create(string: str) -> Path:
        """
        Create the Path without normalizing its string.
        """
        result: Path = Path()
        result._string = string
        return result

    def _get_first_delimiter_pos(self) -> int:
        """
        Returns the first position of a delimiter in the given string.
        If there is no delimiter character found the size of the string is returned.
        """
        for index, char in enumerate(self._string):
            if char in DELIMITERS:
                return index
        return len(self._string)

    def _normalize(self) -> None:
        """
        Checks and normalizes the given Path string before storing it in the Path instance.
        """
        # empty path
        string_length: int = len(self._string)
        if string_length == 0:
            return

        # service paths are taken as-is, because they contain a service-specific URI
        first_delimiter: int = self._get_first_delimiter_pos()
        if first_delimiter < string_length and self._string[first_delimiter] == SERVICE_DELIMITER:
            return

        # if the first delimiter is not the service delimiter, it may not appear anywhere else
        service_delimiter: int = self._string.find(SERVICE_DELIMITER, first_delimiter + 1)
        if service_delimiter != -1:
            raise Path.Error.show_location(
                self._string, service_delimiter, 1,
                f'Non-Service Paths may not contain the Service delimiter "{SERVICE_DELIMITER}"')

        # if the path contains an interop delimiter, ensure that it is followed by a single, valid name
        interop_delimiter: int = self._string.find(INTEROP_DELIMITER, first_delimiter)  # interop might be the first
        if interop_delimiter == -1:
            interop_delimiter = string_length  # denotes the end of the node path
        else:
            # empty interop names are not allowed
            if interop_delimiter == string_length - 1:
                raise Path.Error.show_location(
                    self._string, interop_delimiter, 1, "Empty Interop names are not allowed")

            # additional delimiters of any kind after the interop delimiter are not allowed
            extra_delimiter: int = self._string.find(NODE_DELIMITER, interop_delimiter + 1)
            if extra_delimiter == -1:
                extra_delimiter: int = self._string.find(INTEROP_DELIMITER, interop_delimiter + 1)
            if extra_delimiter != -1:
                raise Path.Error.show_location(
                    self._string, extra_delimiter, string_length - extra_delimiter,
                    "An Interop name must not contain Path control characters")

        # at this point `it_index` is the index of the first delimiter in a node or interop path
        is_absolute: bool = (first_delimiter == 0 and self._string[first_delimiter] == NODE_DELIMITER)

        # split the path into tokens based on the node delimiters
        nodes: List[str] = [] if is_absolute else [self._string[:first_delimiter]]
        if interop_delimiter > first_delimiter + 1:
            nodes.extend(self._string[first_delimiter + 1:interop_delimiter].split(NODE_DELIMITER))

        # normalize the node path
        normalized_nodes: List[str] = []
        for node_name in nodes:
            # ignore empty node names
            if len(node_name) == 0:
                continue

            # ignore STEP IN PLACE in all but the special "self" path: "."
            if node_name == STEP_IN_PLACE and len(nodes) > 1:
                continue

            # depending on the path so far, STEP UP either removes the rightmost normalized name or is simply appended
            elif node_name == STEP_UP:
                if len(normalized_nodes) == 0:
                    if is_absolute:
                        raise Path.Error.show_location(
                            self._string, 0, len(STEP_UP), 'Absolute Path cannot be resolved.')
                    # if the path is relative, store the ".." token at the beginning
                else:  # len(normalized_nodes) > 0
                    if normalized_nodes[-1] != STEP_UP:
                        normalized_nodes.pop()  # if the token is a STEP UP and we know the parent node, go back to it
                        continue
                    assert not is_absolute  # if the last token is already a STEP UP, add the new STEP UP

            # append the next node name to the normalized list
            normalized_nodes.append(node_name)

        # do not construct a new string if the given one is fine
        if len(nodes) == len(normalized_nodes):
            return

        # construct a new path if the given one needed to be normalized
        self._string = \
            f'{NODE_DELIMITER if is_absolute else ""}' \
            f'{NODE_DELIMITER.join(normalized_nodes)}' \
            f'{INTEROP_DELIMITER + self._string[interop_delimiter + 1:] if interop_delimiter < string_length else ""}'

    @staticmethod
    def check_name(name: str):
        """
        A valid Name identifier in notf must not be empty and must not control any Path control characters.
        """
        if not isinstance(name, str):
            raise NameError('Names must be strings')
        if name == "":
            raise NameError('Names may not be empty')
        if name in (STEP_IN_PLACE, STEP_UP):
            raise NameError(f'The name "{name}" is reserved.')
        for path_control_character in (':', '/', '|'):
            if path_control_character in name:
                raise NameError('Names may not contain Path control characters colon [:], slash [/] and pipe [|]')

    def is_empty(self) -> bool:
        """
        Checks whether the Path is empty.
        An absolute path without any Nodes is the root and not empty.
        """
        return len(self._string) == 0

    def is_service_path(self) -> bool:
        """
        Checks whether this Path identifies a Fact from a Service or a Node/Interop in the Scene.
        """
        first_delimiter: int = self._get_first_delimiter_pos()
        return first_delimiter < len(self._string) and self._string[first_delimiter] == SERVICE_DELIMITER

    def is_absolute(self) -> bool:
        """
        An absolute Path does not need to be resolved before it can be used.
        Service Paths are always absolute, Node Paths are absolute iff they begin with a Node delimiter character.
        """
        first_delimiter: int = self._get_first_delimiter_pos()
        return (first_delimiter < len(self._string) and
                ((first_delimiter == 0 and self._string[0] == NODE_DELIMITER) or
                 self._string[first_delimiter] == SERVICE_DELIMITER))

    def is_interop_path(self) -> bool:
        """
        Checks whether or not the last token in the Path identifies an Interop.
        """
        first_delimiter: int = self._get_first_delimiter_pos()
        return (first_delimiter < len(self._string) and
                self._string[first_delimiter] != SERVICE_DELIMITER and
                (self._string[first_delimiter] == INTEROP_DELIMITER or
                 self._string.rfind(INTEROP_DELIMITER, first_delimiter + 1) != -1))

    def is_node_path(self) -> bool:
        """
        Checks whether or not the last token in the Path is a Node.
        """
        if len(self._string) == 0:
            return False
        first_delimiter: int = self._get_first_delimiter_pos()
        if first_delimiter == len(self._string):
            return True
        return (self._string[first_delimiter] not in (SERVICE_DELIMITER, INTEROP_DELIMITER) and
                self._string.rfind(INTEROP_DELIMITER, first_delimiter + 1) == -1)

    def is_relative(self) -> bool:
        """
        Tests if this path is a relative Node Path (not absolute).
        """
        if len(self._string) == 0:
            return True
        first_delimiter: int = self._get_first_delimiter_pos()
        if first_delimiter == len(self._string):
            return True
        return (self._string[0] != NODE_DELIMITER and
                self._string[self._get_first_delimiter_pos()] != SERVICE_DELIMITER)

    def get_node_path(self) -> Optional[Path]:
        """
        Returns this Path as a Node Path (without an Interop postfix).
        If the Path is empty or it is a Service Path, returns None.
        """
        if len(self._string) == 0:
            return None
        first_delimiter: int = self._get_first_delimiter_pos()
        if first_delimiter == len(self._string):
            return self
        if self._string[first_delimiter] == SERVICE_DELIMITER:
            return None
        interop_delimiter: int = self._string.rfind(INTEROP_DELIMITER, first_delimiter + 1)
        return Path._create(self._string if interop_delimiter == -1 else self._string[:interop_delimiter])

    def get_interop(self) -> Optional[str]:
        """
        Returns the name of the Interop pointed to by this Path or None if this is not an Interop Path.
        """
        if len(self._string) == 0:
            return None
        first_delimiter: int = self._get_first_delimiter_pos()
        if first_delimiter == len(self._string):
            return None
        elif self._string[first_delimiter] == SERVICE_DELIMITER:
            return None
        interop_delimiter: int = self._string.rfind(INTEROP_DELIMITER, first_delimiter)
        if interop_delimiter == -1:
            return None
        return self._string[interop_delimiter + 1:]

    def get_iterator(self) -> Iterator[str]:
        return self.__iter__()

    def __len__(self) -> int:
        """
        Number of Nodes in the normalized Path (this excludes the Interop).
        Service Paths contain no Nodes and always return 0.
        """
        if len(self._string) == 0:
            return 0
        first_delimiter: int = self._get_first_delimiter_pos()
        if first_delimiter == len(self._string):
            return 1
        if self._string[first_delimiter] == SERVICE_DELIMITER:
            return 0
        return self._string.count(NODE_DELIMITER, first_delimiter)

    def __str__(self) -> str:
        """
        Build a string representation of this Path.
        """
        return self._string

    def __eq__(self, other: Path):
        """
        Tests two Paths for equality.
        """
        return self._string == other._string

    def __iter__(self) -> Iterator[str]:
        """
        Iterates over all Node names in this Path.
        If this is a service Path, a pure Interop Path or the empty Path, the returned iterator is empty.
        """
        # empty string
        if len(self._string) == 0:
            return

        # path contains one name only
        index: int = self._get_first_delimiter_pos()
        if index == len(self._string):
            yield self._string
            return

        # path is a service or pure interop path
        if self._string[index] in (SERVICE_DELIMITER, INTEROP_DELIMITER):
            return

        # if this is a relative path, yield the first name
        elif index > 0:
            yield self._string[:index]

        # yield subsequent names
        index += 1
        while index < len(self._string):
            next_delimiter: int = self._string.find(NODE_DELIMITER, index)
            if next_delimiter > index:
                yield self._string[index: next_delimiter]
                index = next_delimiter + 1
            else:
                next_delimiter: int = self._string.find(INTEROP_DELIMITER, index)
                if next_delimiter == -1:
                    yield self._string[index:]
                elif index == next_delimiter:
                    return  # root interop
                else:
                    yield self._string[index: next_delimiter]
                break

    def __add__(self, other: Path) -> Path:
        """
        Concatenate this Node or Interop Path with another Node or Interop Path.
        If this is an Interop Path, the Interop will be removed prior to adding the other. If the other Path is an
        Interop Path, the resulting Path will point to the other Interop.
        :param other: Other Path to add.
        :return: New concatenated Path.
        :raise Path.Error: If other is an absolute Path or at least one of the Paths is a Service Path.
        """
        if other.is_absolute():
            raise Path.Error(f'Cannot add absolute Path "{str(other)}" to "{str(self)}"')
        if self.is_service_path() or other.is_service_path():
            raise Path.Error(f'Cannot concatenate a Service Path ("{str(self)}" and "{str(other)}")')

        return Path(str(self.get_node_path()) + NODE_DELIMITER + str(other))

    def __iadd__(self, other: Path) -> Path:
        """
        In-place concatenation of this Path with other.
        :param other: Other Path to add.
        :return: This Path after other was concatenated to it.
        :raise Path.Error: If other is an absolute Path.
        """
        self._string = str(self + other)
        return self
