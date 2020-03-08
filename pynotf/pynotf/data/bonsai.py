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
   the second one is the forward offset to the branch in the array.
   The first child branch does not store an offset because it is always `number of child branches * 2`)
"""
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

    assert len(set(string_names)) == len(string_names)  # names must be unique

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
        result.extend(int(character) for character in common_string)  # may be empty

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

        # create the first child branch
        create_node_recursively(branches[labels[0]])

        # create and fill offset value for the remaining child branches
        for child_index in range(1, len(labels)):
            offset_index: int = children_index + 1 + (2 * child_index)
            offset = len(result) - offset_index - 4  # 4 is the smallest possible offset
            assert offset >= 0
            if offset > 255:
                raise ValueError(f'Cannot encode the given strings in a Bonsai dictionary '
                                 f'because it would require an offset of {offset} (> 255)')
            result[offset_index] = offset
            create_node_recursively(branches[labels[child_index]])

    result: List[int] = []
    create_node_recursively(sorted(((name.encode('utf-8'), index) for index, name in enumerate(string_names)),
                                   key=lambda pair: pair[0]))
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
        number_of_branches_index: int = start_index + common_word_size + 2
        assert number_of_branches_index < len(bonsai)
        number_of_branches: int = bonsai[number_of_branches_index]
        if number_of_branches == 0:
            return None  # word was not found

        # find the child branch to continue traversal
        target_branch_label: int = remaining_word[0]

        # we can either take the first branch...
        left_branch_index: int = number_of_branches_index + 1
        assert left_branch_index < len(bonsai)
        if target_branch_label == bonsai[left_branch_index]:
            return traverse(remaining_word[1:], number_of_branches_index + (number_of_branches * 2))

        if target_branch_label < bonsai[left_branch_index]:
            return None  # target label is smaller than the labels of all child branches

        # ...or the last branch...
        right_branch_index: int = number_of_branches_index + ((number_of_branches - 1) * 2)
        assert right_branch_index + 1 < len(bonsai)
        if target_branch_label == bonsai[right_branch_index]:
            return traverse(remaining_word[1:], right_branch_index + 1 + bonsai[right_branch_index + 1] + 4)

        if target_branch_label > bonsai[right_branch_index]:
            return None  # target label is greater than the labels of all child branches

        # ... or find the correct one using binary search
        left_branch_index += 1  # move right to the leftmost unchecked branch index
        right_branch_index -= 2  # move left to the rightmost unchecked branch index
        while left_branch_index != right_branch_index:
            center_branch_index: int = left_branch_index + (2 * ((right_branch_index - left_branch_index) // 4))
            center_branch_label: int = bonsai[center_branch_index]
            if target_branch_label == center_branch_label:
                branch_offset: int = center_branch_index + 1
                return traverse(remaining_word[1:], branch_offset + bonsai[branch_offset] + 4)
            elif target_branch_label < center_branch_label:
                right_branch_index = center_branch_index
            elif left_branch_index == center_branch_index:
                break
            else:
                left_branch_index = center_branch_index

        # the last possible branch
        if target_branch_label == bonsai[right_branch_index]:
            branch_offset: int = right_branch_index + 1
            return traverse(remaining_word[1:], branch_offset + bonsai[branch_offset] + 4)
        else:
            return None  # target label does not denote a child branch

    return traverse(word.encode('utf-8'))


########################################################################################################################

class Bonsai:
    def __init__(self, names: List[str]):
        self._bonsai: bytes = create_bonsai(names)

    def __getitem__(self, name: str) -> int:
        result: Optional[int] = find_in_bonsai(self._bonsai, name)
        if result is None:
            raise KeyError(f'Could not find "{name}" in Bonsai')
        return result


########################################################################################################################

def main():
    test_case: int = 5
    if test_case == 1:
        names: List[str] = [
            'a',  # 0
            'to',  # 1
            'tea',  # 2
            'ted',  # 3
            'ten',  # 4
            'i',  # 5
            'in',  # 6
            'inn',  # 7
        ]
    elif test_case == 2:
        names: List[str] = [
            'xform',  # 0
            'layout_xform',  # 1
            'claim',  # 2
            'opacity',  # 3
            'period',  # 4
            'progress',  # 5
            'amplitude',  # 6
            'something else',  # 7
        ]
    elif test_case == 3:
        names = ['0', '00']
    elif test_case == 4:
        names = ['0', '0/']
    elif test_case == 5:
        names: List[str] = [
            'xform',
            'layout_xform',
            'claim',
            'opacity',
            'period',
            'progress',
            'amplitude',
            'something else',
            'long ass prop',
            'whatever else',
            'sys.prop.derb',
            'sys.prop.superderb',
            'blub.di.bla',
            'blub.di.bla2'
        ]
    else:
        names: List[str] = []
    bonsai = create_bonsai(names)
    print(bonsai)
    print(f'Length of bonsai: {len(bonsai)}')
    #
    # expected: List[int] = [
    #     0, NO_ID, 3, int(*b'a'), int(*b'i'), 2, int(*b't'), 11,
    #     0, 0, 0,  # a
    #     0, 5, 1, int(*b'n'),  # i
    #     0, 6, 1, int(*b'n'),  # in
    #     0, 7, 0,  # inn
    #     0, NO_ID, 2, int(*b'e'), int(*b'o'), 14,
    #     0, NO_ID, 3, int(*b'a'), int(*b'd'), 2, int(*b'n'), 3,
    #     0, 2, 0,  # tea
    #     0, 3, 0,  # ted
    #     0, 4, 0,  # ten
    #     0, 1, 0,  # to
    # ]
    # print(expected)
    #
    # print(f'Size of the result: {len(bonsai)} bytes (= {len(bonsai) / 8} pointers)')
    #
    # success: str = "YES!" if bonsai == expected else "... nope"
    # print(f'Success? {success}')

    for index, name in enumerate(names):
        result: Optional[int] = find_in_bonsai(bonsai, name)
        if result is None:
            print(f'FAILURE! "{name}" not found. Expected {index}')
        elif result == index:
            print(f'Success. "{name}" -> {result}')
        else:
            print(f'FAILURE! "{name}" -> {result}, but expected {index}')


if __name__ == '__main__':
    main()
