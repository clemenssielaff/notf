import unittest
from typing import List

from hypothesis import given, example
from hypothesis.strategies import composite, floats, lists

# noinspection PyProtectedMember
from pynotf.core.layouts.flexbox import distribute_surplus, LayoutAdapter


# HYPOTHESIS ###########################################################################################################

@composite
def layout_adapter(draw):
    min_size: float = draw(floats(min_value=0, max_value=100))
    preferred: float = draw(floats(min_value=min_size, max_value=min_size + 100))
    max_size: float = draw(floats(min_value=preferred, max_value=preferred + 100))
    scaling: float = draw(floats(min_value=0, max_value=2, exclude_min=True))
    return LayoutAdapter(result=min_size, preferred=preferred, maximum=max_size, scaling=scaling)


########################################################################################################################
# TEST CASE
########################################################################################################################

class TestCase(unittest.TestCase):

    def test_zero_surplus(self):
        """
        Do nothing if there is no surplus to distribute.
        """
        surplus: float = 0
        items: List[LayoutAdapter] = [
            LayoutAdapter(preferred=5, maximum=10, scaling=1, result=5.0),
            LayoutAdapter(preferred=5, maximum=8, scaling=2, result=2.0),
        ]
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 0.)
        self.assertEqual(items[0].result, 5.)
        self.assertEqual(items[1].result, 2.)

    def test_zero_items(self):
        """
        Do nothing if there are no items to distribute to.
        """
        surplus: float = 7
        items: List[LayoutAdapter] = []
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 7.)

    def test_two_equal_items(self):
        """
        Two items separate all available space neatly in half.
        """
        surplus: float = 10
        items: List[LayoutAdapter] = [
            LayoutAdapter(preferred=5, maximum=10, scaling=1, result=2.0),
            LayoutAdapter(preferred=5, maximum=10, scaling=1, result=2.0),
        ]
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 0.)
        self.assertEqual(items[0].result, 5.)
        self.assertEqual(items[1].result, 5.)

    def test_different_maxima(self):
        """
        Two items with the same preferred, but different maximum sizes that both fit in the available space with room
        to spare.
        """
        surplus: float = 20
        items: List[LayoutAdapter] = [
            LayoutAdapter(preferred=4, maximum=8, scaling=2, result=2.0),
            LayoutAdapter(preferred=4, maximum=10, scaling=1, result=3.0),
        ]
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 2.)
        self.assertEqual(items[0].result, 8.)
        self.assertEqual(items[1].result, 10.)

    def test_preferred_too_large(self):
        """
        Two items with preferred sizes that does not fit into the available space
        """
        surplus: float = 10
        items: List[LayoutAdapter] = [
            LayoutAdapter(preferred=8, maximum=10, scaling=2, result=2),
            LayoutAdapter(preferred=8, maximum=10, scaling=1, result=2),
        ]
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 0.)
        self.assertEqual(items[0].result, 6.)
        self.assertEqual(items[1].result, 4.)

    def test_maximum_too_large(self):
        """
        Two items with fitting preferred but too large maximum sizes to fit into the available space.
        """
        surplus: float = 13
        items: List[LayoutAdapter] = [
            LayoutAdapter(preferred=3, maximum=10, scaling=2, result=2),
            LayoutAdapter(preferred=4, maximum=10, scaling=1, result=3),
        ]
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 0.)
        self.assertEqual(items[0].result, 7.)
        self.assertEqual(items[1].result, 6.)

    def test_minimum_too_small(self):
        """
        Two items with a minimal size that is already too large for the space.
        """
        surplus: float = 6
        items: List[LayoutAdapter] = [
            LayoutAdapter(preferred=8, maximum=20, scaling=2, result=3),
            LayoutAdapter(preferred=7, maximum=11, scaling=1, result=4),
        ]
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 0.)
        self.assertEqual(items[0].result, 3.)
        self.assertEqual(items[1].result, 4.)

    def test_no_deficit_or_leeway(self):
        """
        Two items where one has no deficit, the other one no leeway.
        """
        surplus: float = 16
        items: List[LayoutAdapter] = [
            LayoutAdapter(preferred=8, maximum=8, scaling=1, result=3),
            LayoutAdapter(preferred=4, maximum=9, scaling=2, result=4),
        ]
        new_surplus: float = distribute_surplus(surplus, items)
        self.assertEqual(new_surplus, 0.)
        self.assertEqual(items[0].result, 8.)
        self.assertEqual(items[1].result, 8.)

    @given(floats(min_value=0), lists(layout_adapter()))
    @example(40, [LayoutAdapter(preferred=2, maximum=4, scaling=1, result=0.0),
                  LayoutAdapter(preferred=2, maximum=2, scaling=0.1, result=0.0)])
    @example(40, [LayoutAdapter(preferred=2, maximum=4, scaling=1.1, result=0),
                  LayoutAdapter(preferred=30, maximum=30, scaling=0.001, result=0)])
    @example(0.09998100360952836, [LayoutAdapter(preferred=0.0, maximum=0.09998100360932852,
                                                 scaling=0.1, result=0.0)])
    @example(4.440892098500627e-13, [LayoutAdapter(preferred=0.0, maximum=2.2204460492503137e-14,
                                                   scaling=5e-324, result=0.0)])
    @example(4.440892098500627e-13, [LayoutAdapter(preferred=0.0, maximum=2.2204460492503137e-14,
                                                   scaling=1.3322676295501882e-15, result=0.0)])
    @example(4.440892098500627e-13, [LayoutAdapter(preferred=2.2204460492503137e-14, maximum=2,
                                                   scaling=0.000001, result=0.0),
                                     LayoutAdapter(preferred=2.2204460492503137e-14, maximum=4,
                                                   scaling=0.000001, result=0.0)])
    @example(4.440892098500627e-13, [LayoutAdapter(preferred=2.2204460492503137e-14, maximum=2.2204460492503137e-14,
                                                   scaling=5e-324, result=0.0),
                                     LayoutAdapter(preferred=0.0, maximum=2.2204460492503137e-14,
                                                   scaling=4.440892098500627e-16, result=0.0)])
    @example(123.71223217124962, [LayoutAdapter(preferred=99.5510028775964, maximum=99.55100287759643,
                                                scaling=5.678470721799709e-05, result=46.58168792776559),
                                  LayoutAdapter(preferred=24.161229293653413, maximum=24.161229293653413,
                                                scaling=1e-06, result=24.161229293653392)])
    @example(124.17620922158615, [LayoutAdapter(preferred=53.699713919319706, maximum=53.69971391931973,
                                                scaling=0.011579122603096405, result=43.68692374206679),
                                  LayoutAdapter(preferred=23.9289729304145, maximum=23.9289729304145,
                                                scaling=0.0010000000000000002, result=23.9289729304145),
                                  LayoutAdapter(preferred=46.5475223729949, maximum=46.5475223729949,
                                                scaling=0.0010000000000000002, result=46.54752237299488)])
    @example(150.5327587203307, [LayoutAdapter(preferred=76.11293396667824, maximum=76.11293396667824,
                                               scaling=0.0010000000000000002, result=6.681876015846535),
                                 LayoutAdapter(preferred=26.823737116113307, maximum=26.823737116113307,
                                               scaling=0.0010000000000000002, result=26.823737116113307),
                                 LayoutAdapter(preferred=47.59608763753919, maximum=47.59608864175943,
                                               scaling=0.07086230594719127, result=46.58168792776555)])
    @example(7.698356841659317, [LayoutAdapter(preferred=34.5170040315632, maximum=34.5170040315632,
                                               scaling=0.0010000000000000002, result=6.6767909571282775),
                                 LayoutAdapter(preferred=2.2204460492503137e-14, maximum=0.00010093146514122966,
                                               scaling=0.04660077377209058, result=0.0),
                                 LayoutAdapter(preferred=0.0, maximum=0.0,
                                               scaling=0.0010000000000000002, result=0.0)])
    def test_random(self, surplus: float, items):
        """
        Tests generated by Hypothesis.
        If this ever comes across a failure case, add it to the list of "example" decorators as regression test.
        """
        leftover_surplus: float = distribute_surplus(surplus, items)

        self.assertLessEqual(0., leftover_surplus)
        self.assertLessEqual(leftover_surplus, surplus)

        below_preference_count: int = 0
        exact_preference_count: int = 0
        above_preference_count: int = 0

        for item in items:
            # the size of each item has to be 0 <= size <= maximum
            self.assertLessEqual(0., item.result)
            self.assertLessEqual(item.result, item.maximum)

            if item.result > item.preferred:
                above_preference_count += 1
            elif item.result < item.preferred:
                below_preference_count += 1
            else:
                exact_preference_count += 1

        # all items have to be either below/equal or above/equal to their preference
        self.assertTrue((below_preference_count + exact_preference_count == len(items)) or
                        (above_preference_count + exact_preference_count == len(items)))
