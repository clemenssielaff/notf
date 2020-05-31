import unittest
from typing import List

from hypothesis import given, example
from hypothesis.strategies import composite, lists, text

from pynotf.data import Bonsai


# HYPOTHESIS ###########################################################################################################

@composite
def unique_strings(draw):
    return draw(lists(text(), unique=True, max_size=10))


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def _check_bonsai(self, names: List[str]):
        """
        Testing a Bonsai is straightforward.
        Generate one, then see if all indices match.
        """
        bonsai: Bonsai = Bonsai(names)
        for index, name in enumerate(names):
            self.assertEqual(bonsai[name], index)
        self.assertEqual(names, bonsai.keys())

    def test_generic(self):
        """
        A short list of names I would expect to be stored in a Bonsai.
        """
        self._check_bonsai([
            'xform',  # 0
            'layout_xform',  # 1
            'claim',  # 2
            'opacity',  # 3
            'period',  # 4
            'progress',  # 5
            'amplitude',  # 6
            'something else',  # 7
        ])

    def test_no_names(self):
        """
        What it says in the function name.
        """
        self._check_bonsai([])
        self.assertIsNone(Bonsai([]).get('anything'))

    def test_empty_name(self):
        """
        What happens if a name is empty from the start.
        """
        bonsai: Bonsai = Bonsai(['a', '', 'b'])
        self.assertEqual(bonsai[''], 1)

    def test_not_unique(self):
        """
        If you construct a Bonsai with duplicate names.
        """
        with self.assertRaises(ValueError):
            Bonsai(['a', 'a', 'b'])

    # noinspection PyTypeChecker
    def test_not_found(self):
        """
        Generates a Bonsai and tries to find a name that is not part of it.
        """
        bonsai: Bonsai = Bonsai(['This', 'is', 'a', 'Bonsai'])
        with self.assertRaises(KeyError):
            _ = bonsai['ThisIsNoInTheBonsai']
        with self.assertRaises(KeyError):
            _ = bonsai[123]
        self.assertIsNone(bonsai.get('A'))
        self.assertIsNone(bonsai.get('iS'))
        self.assertIsNone(bonsai.get('z'))
        self.assertIsNone(bonsai.get(456))

    def test_contains(self):
        """
        Check if a name is part of the Bonsai.
        """
        bonsai: Bonsai = Bonsai(['a', 'b'])
        self.assertTrue('b' in bonsai)
        self.assertFalse('c' in bonsai)

    def test_wikipedia_trie(self):
        """
        See https://en.wikipedia.org/wiki/Trie
        """
        self._check_bonsai([
            'a',  # 0
            'to',  # 1
            'tea',  # 2
            'ted',  # 3
            'ten',  # 4
            'i',  # 5
            'in',  # 6
            'inn',  # 7
        ])

    def test_long(self):
        """
        When absolute offsets were stored, this Bonsai had an offset > 255, but now it works fine.
        """
        self._check_bonsai([
            'xform',
            'layout_xform',
            'claim',
            'opacity',
            'period',
            'reverse_opacity',
            'state',
            'progress',
            'phase',
            'seed',
            'width',
            'height',
            'score',
            'amplitude',
            't',
            'i',
            'value',
            'id',
            'parent_id',
            'strength',
            'power_level',
            'child_count',
            'data_offset'
            'color_r',
            'color_g',
            'color_b',
            'color_space',
            'is_critical',
            'is_reversible',
        ])

    def test_name_too_long(self):
        """
        Bonsais can not store names that are larger than 255 characters
        """
        with self.assertRaises(ValueError):
            _ = Bonsai([  # 4x64 = 256 characters long
                'LoremipsumdolorsitametconsecteturadipiscingelitDonecelitnuncsoll'
                'icitudinornarehesndreriteupellentesqueiddiamIntegerapulvinarleoP'
                'ellentesqueacefficiturloremAeneansollicitudinpharetraornareNulla'
                'auctordapibuslectussedmollisFuscenecsemeuismodelementumexetposes'
            ])

    def test_offset_too_large(self):
        """
        Offsets within a Bonsai can not be larger than 255.
        """
        with self.assertRaises(ValueError):
            _ = Bonsai([
                'A',  # 65
                'LoremipsumdolorsitametconsecteturadipiscingelitDonecelitnuncsoll'
                'icitudinornarehesndreriteupellentesqueiddiamIntegerapulvinarleoP'
                'ellentesqueacefficiturloremAeneansollicitudinpharetraornareNulla'
                'auctordapibuslectussedmollisFuscenecsemeuismodelementumexetpos',
                'z'  # 122
            ])

    @given(unique_strings())
    @example(['0', '00'])
    @example(['0', '0/'])
    def test_random(self, strings: List[str]):
        """
        Tests generated by Hypothesis.
        If this ever comes across a failure case, add it to the list of "example" decorators as regression test.
        """
        self._check_bonsai(strings)
