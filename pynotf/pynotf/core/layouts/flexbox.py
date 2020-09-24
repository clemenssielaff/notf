from __future__ import annotations

from typing import Dict

from pycnotf import Size2f, Aabrf, M3f
from pynotf.data import Value, RowHandle, Claim
import pynotf.core as core

from pynotf.core.layout import Layout, LayoutComposition, NodeComposition, NodeCompositions, LayoutIndex

__all__ = ('LtFlexbox',)


# FLEXBOX ##############################################################################################################

class LtFlexbox:
    @staticmethod
    def create(args: Value) -> RowHandle:
        return core.get_app().get_table(core.TableIndex.LAYOUTS).add_row(
            layout_index=LayoutIndex.FLEXBOX,
            args=args,
            node_value_initial=Value(),
        )

    @staticmethod
    def layout(self: Layout, grant: Size2f) -> LayoutComposition:
        compositions: Dict[str, NodeComposition] = {}
        x_offset: float = 0
        for node_view in self.get_nodes():
            node_grant: Size2f = Size2f(
                width=max(node_view.claim.horizontal.min, min(node_view.claim.horizontal.max, grant.width)),
                height=max(node_view.claim.vertical.min, min(node_view.claim.vertical.max, grant.height)),
            )
            assert node_view.name not in compositions
            compositions[node_view.name] = NodeComposition(
                xform=M3f(e=x_offset, f=10),  # TODO: f=10 is a hack .. just like this class
                grant=node_grant,
                opacity=node_view.opacity,
            )
            x_offset += node_grant.width + float(self.get_argument('margin'))

        return LayoutComposition(
            nodes=NodeCompositions(compositions),
            order=self.get_order(),
            aabr=Aabrf(grant),
            claim=Claim(),
        )
