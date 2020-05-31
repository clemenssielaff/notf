"""
Z85 encoding is like base64 encoding but better.
See the specs at https://rfc.zeromq.org/spec/32/
For an overview of Ascii85 encoding see https://en.wikipedia.org/wiki/Ascii85#Overview

This implementation is an improved version of the implementation in the first comment of
https://gist.github.com/minrk/6357188
which is based on the code above, licensed under the New BSD License.

For the original reference implementation in C see:
https://github.com/zeromq/rfc/blob/master/src/spec_32.c
"""
from __future__ import annotations

from typing import List, Dict, Tuple, Optional
from struct import pack, unpack

# CONSTANTS ############################################################################################################

# Z85CHARS is the base 85 symbol table
Z85CHARS: bytes = b"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#"

# Z85MAP maps characters in the base 85 symbol table to their index in the table
Z85MAP: Dict[int, int] = {char: index for index, char in enumerate(Z85CHARS)}

FIVE_POWERS_OF_85s: Tuple[int, ...] = tuple(85 ** i for i in range(5))

TWO_TO_THE_POWER_OF_32: int = 2 ** 32


# Z85 ###################################################################################$$$$$$#########################

class Z85:
    class DecodeError(Exception):
        pass

    @staticmethod
    def encode(raw_bytes: bytes) -> bytes:
        """
        Encodes raw bytes into Z85b.
        See https://en.wikipedia.org/wiki/Ascii85#Overview for details.
        """
        if raw_bytes == b'':
            return raw_bytes  # empty in -> empty out
        padding: int = 3 - ((len(raw_bytes) - 1) % 4)
        raw_bytes += b'\x00' * padding  # pad to a multiple of 4
        encoded: bytearray = bytearray()
        for four_bytes in unpack(f'<{len(raw_bytes) // 4}I', raw_bytes):
            for power_of_85 in FIVE_POWERS_OF_85s:
                encoded.append(Z85CHARS[(four_bytes // power_of_85) % 85])
        return bytes(encoded)[:-padding or None]

    @staticmethod
    def decode(z85_bytes: bytes) -> bytes:
        """
        Decode Z85b bytes to raw bytes.
        """
        if z85_bytes == b'':
            return z85_bytes  # empty in -> empty out
        padding: int = 0
        values: List[int] = []
        for five_offset in range(0, len(z85_bytes), 5):
            value: int = 0
            for offset, power_of_85 in enumerate(FIVE_POWERS_OF_85s):
                position: int = five_offset + offset
                if position >= len(z85_bytes):
                    padding = 4 - offset
                    break
                byte_code: int = z85_bytes[position]
                z85: Optional[int] = Z85MAP.get(byte_code)
                if z85 is None:
                    raise Z85.DecodeError(
                        'Invalid byte code {0} in\n'
                        '\t"{1}"\n'
                        '\t{2:>{3}}\n'.format(
                            byte_code,
                            str(z85_bytes)[2:-1],
                            '^^^^', position + 5,
                        )
                    )
                value += z85 * power_of_85
            if value >= TWO_TO_THE_POWER_OF_32:
                word: str = z85_bytes[five_offset: five_offset + 5 - padding].decode()
                raise Z85.DecodeError(f'Z85b encoded word "{word}" => {value} '
                                      f'exceeds range of uint32: "0cSn%" => 4294967295')
            values.append(value)
        return pack(f'<{len(values)}I', *values)[:-padding or None]
