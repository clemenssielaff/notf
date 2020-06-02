"""
CODENAME BONSAI

_[Bonsai](https://en.wikipedia.org/wiki/Bonsai), because it is a miniature [Trie](https://en.wikipedia.org/wiki/Trie)_

A self-contained, minimal and POD data structure that provides the fastest possible (?) lookup of numeric Ids by name.
Names are UTF-8 strings, and with a word size of 8 bits you can have at most 255 words in a dictionary.

A Bonsai dictionary is a simple array of "nodes". Each node in the array contains the following
(potentially zero-length = empty) fields:

1. The size of the "common string" in words.
2. The actual common string, is potentially empty.
3. Numeric Id, if this node signifies the end of a name.
4. The number of child branches.
5. (for each child node) A pair of words: the first one is the label of the branch,
   the second one is the forward offset to the branch in the array from the end of this node.
   The first child branch does not store an offset because it is always zero.

Instead of storing the absolute offset value, each offset is only the offset added by this child branch. In order to
calculate the actual offset, you need to sum up the offset of all branches and including the target branch's.
"""
from __future__ import annotations
from typing import List, Tuple, Sequence, Dict, Optional

WORD_SIZE: int = 8  # bits
NO_ID: int = (2 ** WORD_SIZE) - 1


def find_common_string_length(names: Sequence[bytes]) -> int:
    assert len(names) > 0
    reference_name: bytes = names[0]
    common_string_length: int = len(reference_name)
    for name in names[1:]:
        common_string_length = min(common_string_length, len(name))
        for i in range(common_string_length):
            if name[i] != reference_name[i]:
                if i == 0:
                    return 0
                else:
                    common_string_length = i
                    break
    return common_string_length


def create_bonsai(string_names: List[str]) -> bytes:
    if len(string_names) == 0:
        return b''

    if len(set(string_names)) != len(string_names):
        raise ValueError("Names stored in a Bonsai must be unique")

    for string_name in string_names:
        if len(string_name) > NO_ID:
            raise ValueError(f'Names stored in a Bonsai must be <= {NO_ID} characters in size.\n'
                             f'"{string_name}" is {len(string_name)} long.')

    def create_node_recursively(names: List[Tuple[bytes, int]]):
        assert len(names) > 0

        # if there is a single name left, store all of it in this node
        if len(names) == 1:
            common_string, node_id = names[0]
            common_string_length: int = len(common_string)
            names.clear()

        else:
            # find the common substring at the start of all names of the input set
            common_string_length: int = find_common_string_length([name for name, _ in names])
            common_string: bytes = names[0][0][:common_string_length]

            # check to see if the empty name is part of the input set
            node_id: int = NO_ID  # if the empty name is part of the input set, this node has its id
            for name, index in names:
                if len(name) == common_string_length:
                    node_id = index
                    names.remove((name, index))
                    break

        # define the first part of the node: (common string size, common string)
        result.append(common_string_length)
        result.extend(list(common_string))  # may be empty

        # define the node index, may be NO ID
        result.append(node_id)

        # if there are no children, return here
        if len(names) == 0:
            result.append(0)
            return

        # find the child branches
        children_index: int = len(result)
        branches: Dict[int, List[Tuple[bytes, int]]] = {}
        for name, index in names:
            # remove the common string from the start of all names (they are still unique) and place the remainder
            # into buckets identified by the first letter
            first_letter: int = int(name[common_string_length])
            if first_letter in branches:
                branches[first_letter].append((name[common_string_length + 1:], index))
            else:
                branches[first_letter] = [(name[common_string_length + 1:], index)]

        # store the number of child branches
        result.append(len(branches))

        # the first child does not need an offset, since we can calculate it from the number of siblings
        labels: List[int] = sorted(branches.keys())
        result.append(labels[0])

        # the remaining children need an offset, we are going to put zero in it for now and fill it up recursively later
        for label in labels[1:]:
            result.extend((label, 0))
        node_end_index: int = len(result)  # index one past the end of this node

        # create the first child branch
        create_node_recursively(branches[labels[0]])

        # create and fill offset value for the remaining child branches
        last_offset: int = 0
        for child_index in range(1, len(labels)):
            offset_index: int = children_index + 1 + (2 * child_index)
            actual_offset: int = len(result) - node_end_index
            offset: int = actual_offset - last_offset
            assert offset >= 0
            if offset > NO_ID:
                raise ValueError(f'Cannot encode the given strings in a Bonsai dictionary '
                                 f'because it would require an offset of {offset} (> {NO_ID})')
            result[offset_index] = offset
            last_offset = actual_offset
            create_node_recursively(branches[labels[child_index]])

    result: List[int] = []
    create_node_recursively(sorted(
        ((name.encode(), index) for index, name in enumerate(string_names)), key=lambda pair: pair[0]))
    return bytes(result)


def find_in_bonsai(bonsai: bytes, word: str) -> Optional[int]:
    if len(bonsai) == 0:
        return None  # we know that we will not find the word in an empty bonsai

    def traverse(remaining_word: bytes, start_index: int = 0) -> Optional[int]:
        assert start_index < len(bonsai)

        # subtract the common string from the beginning of the remaining word
        common_word_size: int = bonsai[start_index]
        if common_word_size > 0:
            common_string: bytes = bytes(bonsai[start_index + 1: start_index + 1 + common_word_size])
            if not remaining_word.startswith(common_string):
                return None  # word was not found
            remaining_word = remaining_word[common_word_size:]

        # if the word has been exhausted we have ended our journey
        if len(remaining_word) == 0:
            id_index: int = start_index + common_word_size + 1
            assert id_index < len(bonsai)
            result: int = bonsai[id_index]
            return None if result == NO_ID else result  # word was found but might not have an associated id

        # if the node has no child branches, there is nowhere to go from here
        child_count_index: int = start_index + common_word_size + 2
        assert child_count_index < len(bonsai)
        child_count: int = bonsai[child_count_index]
        if child_count == 0:
            return None  # word was not found

        # find the child branch to continue traversal
        target_branch_label: int = remaining_word[0]
        node_end_index: int = child_count_index + (child_count * 2)
        assert node_end_index - 1 < len(bonsai)

        # we can either take the first branch...
        branch_index: int = child_count_index + 1
        if target_branch_label == bonsai[branch_index]:
            return traverse(remaining_word[1:], node_end_index)

        if target_branch_label < bonsai[branch_index]:
            return None  # target label is smaller than the labels of all child branches

        # ...or do a linear search through the remaining child branches
        branch_index += 1  # advance to next unchecked branch index
        total_offset: int = 0
        while branch_index < node_end_index:
            total_offset += bonsai[branch_index + 1]
            if target_branch_label == bonsai[branch_index]:
                return traverse(remaining_word[1:], node_end_index + total_offset)
            else:
                branch_index += 2

        return None  # target label does not denote a child branch

    return traverse(word.encode('utf-8'))


def get_names_from_bonsai(bonsai: bytes) -> List[str]:
    if len(bonsai) == 0:
        return []

    result: Dict[int, str] = {}

    def traverse(name: bytes, node_index: int) -> None:
        assert node_index < len(bonsai)
        common_word_size: int = bonsai[node_index]
        common_word_index: int = node_index + 1
        id_index: int = common_word_index + common_word_size

        # add the common word to the name
        if common_word_size > 0:
            assert node_index + common_word_size < len(bonsai)
            name = name + bonsai[common_word_index: id_index]

        # if this node has an id, it is a new name
        node_id: int = bonsai[id_index]
        if node_id != NO_ID:
            assert node_id not in result
            result[node_id] = name.decode('utf-8')

        # no children
        child_count_index: int = id_index + 1
        assert child_count_index < len(bonsai)
        child_count: int = bonsai[child_count_index]
        if child_count == 0:
            return

        # single child
        node_end_index: int = child_count_index + (child_count * 2)
        label_index: int = child_count_index + 1
        assert node_end_index < len(bonsai)
        traverse(name + bytes(bonsai[label_index: label_index + 1]), node_end_index)

        # multiple children
        label_index += 1
        total_offset: int = 0
        for _ in range(1, child_count):
            offset_index: int = label_index + 1
            total_offset += bonsai[offset_index]
            traverse(name + bytes(bonsai[label_index: offset_index]), node_end_index + total_offset)
            label_index += 2

    traverse(b'', 0)
    return [result[key] for key in sorted(result.keys())]


########################################################################################################################

class Bonsai:
    def __init__(self, names: Optional[List[str]] = None):
        """
        Constructor.
        If no names are given, this constructs an empty Bonsai.
        :param names: All names (in order) to store in the Bonsai.
        """
        self._bonsai: bytes = create_bonsai(names or [])

    def __getitem__(self, name: str) -> int:
        if not isinstance(name, str):
            raise KeyError(f'Can not find "{name}" in Bonsai - only strings are allowed as key.')
        result: Optional[int] = find_in_bonsai(self._bonsai, name)
        if result is None:
            raise KeyError(f'Can not find "{name}" in Bonsai.')
        return result

    def __contains__(self, item: str) -> bool:
        return self.get(item) is not None

    def __eq__(self, other: Bonsai) -> bool:
        if not isinstance(other, Bonsai):
            return False
        return self._bonsai == other._bonsai

    def get(self, name: str) -> Optional[int]:
        if not isinstance(name, str):
            return None
        return find_in_bonsai(self._bonsai, name)

    def keys(self) -> List[str]:
        return get_names_from_bonsai(self._bonsai)

    def is_empty(self) -> bool:
        return len(self._bonsai) == 0
