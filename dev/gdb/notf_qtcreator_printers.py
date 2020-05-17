############################################################################
#
# Copyright (C) 2016 The Qt Company Ltd.
# Contact: https://www.qt.io/licensing/
#
# This file is part of Qt Creator.
#
# Commercial License Usage
# Licensees holding valid commercial Qt licenses may use this file in
# accordance with the commercial license agreement provided with the
# Software or, alternatively, in accordance with the terms contained in
# a written agreement between you and The Qt Company. For licensing terms
# and conditions see https://www.qt.io/terms-conditions. For further
# information use the contact form at https://www.qt.io/contact-us.
#
# GNU General Public License Usage
# Alternatively, this file may be used under the terms of the GNU
# General Public License version 3 as published by the Free Software
# Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
# included in the packaging of this file. Please review the following
# information to ensure the GNU General Public License requirements will
# be met: https://www.gnu.org/licenses/gpl-3.0.html.
#
############################################################################

# This is a place to add your own dumpers for testing purposes.
# Any contents here will be picked up by GDB, LLDB, and CDB based
# debugging in Qt Creator automatically.

# NOTE: This file will get overwritten when updating Qt Creator.
#
# To add dumpers that don't get overwritten, copy this file here
# to a safe location outside the Qt Creator installation and
# make this location known to Qt Creator using the Debugger >
# Locals & Expressions > Extra Debugging Helpers setting.

# Example to display a simple type
# template<typename U, typename V> struct MapNode
# {
#     U key;
#     V data;
# }
#
# def qdump__MapNode(d, value):
#    d.putValue("This is the value column contents")
#    d.putNumChild(2)
#    if d.isExpanded():
#        with Children(d):
#            # Compact simple case.
#            d.putSubItem("key", value["key"])
#            # Same effect, with more customization possibilities.
#            with SubItem(d, "data")
#                d.putItem("data", value["data"])

# Check http://doc.qt.io/qtcreator/creator-debugging-helpers.html
# for more details or look at qttypes.py, stdtypes.py, boosttypes.py
# for more complex examples.
############################################################################

import re
from dumper import *

vector_type_regex = re.compile(r"^notf::detail::Vector(\d)<([^,]*).*>$")

type_dict = {
    "float": {
        "postfix": "f",
        "type": float,
    },
    "double": {
        "postfix": "d",
        "type": float,
    },
    "int": {
        "postfix": "i",
        "type": int,
    },
    "short": {
        "postfix": "s",
        "type": int,
    },
}


def extract_notf_vector_type(full_name):
    matches = re.match(vector_type_regex, full_name)
    if matches is None or len(matches.groups()) != 2:
        raise ValueError("`{}` does not match the regex for notf values".format(full_name))
    dimensions, type_name = matches.groups()

    if type_name not in type_dict:
        raise IndexError("`{}` is an unknown type name".format(type_name))

    return int(dimensions), type_dict[type_name]["postfix"], type_dict[type_name]["type"]


def format_notf_vector_unsafe(dumper, data, typename):
    # extract relevant information about the vector type
    dimensions, postfix, value_type = extract_notf_vector_type(typename)

    # display the type
    dumper.putType("notf::V{}{}".format(dimensions, postfix))

    # display the value
    if value_type == float:
        values = [data[index].floatingPoint() for index in range(dimensions)]
        values_fmt = ", ".join(["{:.6g}" for _ in range(dimensions)])
    elif value_type == int:
        values = [data[index].integer() for index in range(dimensions)]
        values_fmt = ", ".join(["{}" for _ in range(dimensions)])
    else:
        raise TypeError
    dumper.putValue(values_fmt.format(*values))

    # list the vector's children
    dumper.putNumChild(dimensions)
    if dumper.isExpanded():
        with Children(dumper):
            for name, value in zip("xyzw", [data[i] for i in range(dimensions)]):
                dumper.putSubItem(name, value)


def format_notf_vector(dumper, data, typename):
    try:
        format_notf_vector_unsafe(dumper, data, typename)
    except Exception as e:
        dumper.putValue(str(e))
        #dumper.putValue("unknown notf value type")


def qdump__notf__detail__Vector2(dumper, vector):
    dumper.putType('pycnotf::V2f')
    dumper.putValue(f'{vector["data"]["_M_elems"][0].floatingPoint()}, {vector["data"]["_M_elems"][1].floatingPoint()}')
    # format_notf_vector(dumper, vector["data"]["__elems_"], vector.type.name)

def qdump__notf__detail__Vector3(dumper, vector):
    format_notf_vector(dumper, vector["data"]["__elems_"], vector.type.name)


def qdump__notf__detail__Vector4(dumper, vector):
    format_notf_vector(dumper, vector["data"]["__elems_"], vector.type.name)
