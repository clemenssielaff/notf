from typing import List, Optional

from hypothesis import given
from hypothesis.strategies import composite, lists, text

from pynotf.data import Bonsai

@composite
def unique_strings(draw):
    return draw(lists(text(), unique=True))


@given(unique_strings())
def test_bonsai(names: List[str]):
    bonsai: Bonsai = Bonsai(names)
    for index, name in enumerate(names):
        result: Optional[int] = bonsai[name]
        if result != index:
            raise ValueError(f'"{name}" -> {result}, but expected {index}')


if __name__ == "__main__":
    test_bonsai()
