import sys
from typing import List, Tuple, Sequence, Dict

NO_ID: int = sys.maxsize


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


def heap_it(string_names: List[str]) -> List[int]:
    assert len(set(string_names)) == len(string_names)  # names must be unique

    def create_node_recursively(names: List[Tuple[bytes, int]]):
        assert len(names) > 0

        # check to see if the empty name is part of the input set
        node_id: int = NO_ID  # if the empty name is part of the input set, this node has its id
        for name, index in names:
            if name == b"":
                node_id = index
                names.remove((name, index))
                break

        # if the empty name is not part of the set, find the common substring at the start of all names of the input set
        if node_id == NO_ID:
            common_string_length: int = find_common_string_length([name for name, _ in names])
            common_string: bytes = names[0][0][:common_string_length]
        else:
            common_string_length: int = 0
            common_string: bytes = b''

        # define the first part of the node: (common string size, common string)
        result.append(len(common_string))
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
            first_letter: int = int(name[max(0, common_string_length - 1)])
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
            result.extend((0, label))

        # create the first child branch
        create_node_recursively(branches[labels[0]])

        # create and fill offset value for the remaining child branches
        for child_index in range(1, len(labels)):
            offset_index: int = children_index + (2 * child_index)
            result[offset_index] = len(result) - offset_index
            create_node_recursively(branches[labels[child_index]])

    result: List[int] = []
    create_node_recursively(sorted(((name.encode('utf-8'), index) for index, name in enumerate(string_names)),
                                   key=lambda pair: pair[0]))
    return result


if __name__ == '__main__':
    r = heap_it([
        'a',     # 0
        'to',    # 1
        'tea',   # 2
        'ted',   # 3
        'ten',   # 4
        'i',     # 5
        'in',    # 6
        'inn',   # 7
    ])
    print(r)

    expected: List[int] = [
        0, NO_ID, 3, int(*b'a'), 7, int(*b'i'), 16, int(*b't'),
        0, 0, 0,                                                        # a
        0, 5, 1, int(*b'n'),                                            # i
        0, 6, 1, int(*b'n'),                                            # in
        0, 7, 0,                                                        # inn
        0, NO_ID, 2, int(*b'e'), 19, int(*b'o'),
        0, NO_ID, 3, int(*b'a'), 7, int(*b'd'), 8, int(*b'n'),
        0, 2, 0,                                                        # tea
        0, 3, 0,                                                        # ted
        0, 4, 0,                                                        # ten
        0, 1, 0,                                                        # to
    ]
    print(expected)

    print(f'Size of the result: {len(r)} bytes (= {len(r)/8} pointers)')

    success: str = "YES!" if r == expected else "... nope"
    print(f'Success? {success}')