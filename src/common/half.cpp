/**
 * Code for the notf half implementation is inspired by glm, in particular:
 * https://github.com/g-truc/glm/blob/master/glm/detail/type_half.inl
 *
 ** GLM license ********************************************************************************************************
 *
 * The MIT License
 * Copyright (c) 2005 - 2017 G-Truc Creation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "common/half.hpp"

#include <iostream>
#include <stdexcept>
#include <typeinfo>

namespace { // anonymous

union uif32 {
    float f;
    uint32_t i;
};

} // namespace

namespace notf {

using uint = unsigned int;

static_assert(std::is_pod<half>::value, "This compiler does not recognize notf::half as a POD.");

half::half(const float input)
{
    // disassemble the input into the sign, the exponent, and the mantissa
    int sign, exponent, mantissa;
    {
        uif32 converter;
        converter.f = input;

        // the floating point number in `input` is represented by the bit pattern in integer `pattern`
        const int pattern = static_cast<int>(converter.i);

        // shift `sign` into the position where it will go in in the resulting half number
        sign = (pattern >> 16) & 0x00008000;

        // adjust `exponent`, accounting for the different exponent bias of float and half (127 versus 15)
        exponent = ((pattern >> 23) & 0x000000ff) - (127 - 15);

        mantissa = pattern & 0x007fffff;
    }

    // reassemble the sign, exponent and mantissa into the store value
    if (exponent <= 0) {
        if (exponent < -10) {
            // the absolute input is less than the smallest value expressable in a half
            // (maybe a small normalized float, a denormalized float or zero)

            // half is a (signed) zero
            value = static_cast<short>(sign);
        }
        else {
            // the input is a normalized float whose magnitude is too small, producing a denormalized half
            mantissa = (mantissa | 0x00800000) >> (1 - exponent);

            // Round the mantissa to nearest, round "0.5" up.
            // Rounding may cause the significand to overflow and make the result normalized.
            // Because of the way a half's bits are laid out, we don't have to treat this case separately,
            // the code below will handle it correctly.
            if (mantissa & 0x00001000) {
                mantissa += 0x00002000;
            }

            // assemble the result from the sign, exponent (zero) and mantissa
            value = static_cast<short>(sign | (mantissa >> 13));
        }
    }
    else if (exponent == 0xff - (127 - 15)) {
        if (mantissa == 0) {
            // the input is an infinity, producing a infinite half of the same sign
            value = static_cast<short>(sign | 0x7c00);
        }
        else {
            // the input is a NAN

            // Return a half NAN that preserves the sign bit and the 10 leftmost bits of the significand of f,
            // with one exception:
            // If the 10 leftmost bits are all zero, the NAN would turn into an infinity, so we have to set at least one
            // bit in the significand.
            mantissa >>= 13;
            value = static_cast<short>(sign | 0x7c00 | mantissa | (mantissa == 0));
        }
    }
    else {
        // the input is a normalized float

        // round the mantissa to nearest, round "0.5" up
        if (mantissa & 0x00001000) {
            mantissa += 0x00002000;
            if (mantissa & 0x00800000) {
                mantissa = 0;  // overflow in significand,
                exponent += 1; // adjust exponent
            }
        }

        // exponent overflow
        if (exponent > 30) {
            throw std::bad_cast();
        }

        // assemble the result from the sign, exponent and mantissa
        value = static_cast<short>(sign | (exponent << 10) | (mantissa >> 13));
    }
}

half::operator float() const
{
    int sign     = (value >> 15) & 0x00000001;
    int exponent = (value >> 10) & 0x0000001f;
    int mantissa = value & 0x000003ff;
    uif32 converter;

    if (exponent == 0) {
        if (mantissa == 0) {
            // plus or minus zero
            converter.i = static_cast<uint>(sign << 31);
            return converter.f;
        }
        else {
            // denormalized number -- renormalize it
            while (!(mantissa & 0x00000400)) {
                mantissa <<= 1;
                exponent -= 1;
            }
            exponent += 1;
            mantissa &= ~0x00000400;
        }
    }
    else if (exponent == 31) {
        if (mantissa == 0) {
            // positive or negative infinity
            converter.i = static_cast<uint>((sign << 31) | 0x7f800000);
            return converter.f;
        }
        else {
            // nan -- preserve sign and significand bits
            converter.i = static_cast<uint>((sign << 31) | 0x7f800000 | (mantissa << 13));
            return converter.f;
        }
    }

    // normalized number
    exponent = exponent + (127 - 15);
    mantissa = mantissa << 13;

    // assemble the result from the sign, exponent and mantissa
    converter.i = static_cast<uint>((sign << 31) | (exponent << 23) | mantissa);
    return converter.f;
}

//====================================================================================================================//

std::ostream& operator<<(std::ostream& out, const half& value) { return out << static_cast<float>(value); }

} // namespace notf
