#pragma once

// This code is bascially a NoTF-ed version of Boost rational.hpp, which is why I include the original license text of
// that file here:
//
//  (C) Copyright Paul Moore 1999. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or
//  implied warranty, and with no claim as to its suitability for any purpose.

#include "common/integer.hpp"

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

/// Exception thrown when you try to constuct an invalid rational number.
NOTF_EXCEPTION_TYPE(no_rational_error);

// ================================================================================================================== //

/// A rational number consisting of an integer fraction.
template<class Integer, typename = notf::enable_if_t<std::is_integral<Integer>::value>>
class Rational {

    // types -------------------------------------------------------------------------------------------------------- //
public:
    /// Element type.
    using element_t = Integer;

    // methods ------------------------------------------------------------------------------------------------------ //
public:
    /// Default constructor, constructs a valid zero Rational.
    constexpr Rational() = default;

    /// Value constructor.
    /// @param num  Numerator, Denominator defaults to one.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    explicit constexpr Rational(const T& num) : m_num(static_cast<Integer>(num))
    {}

    /// Value constructor.
    /// @param num  Numerator.
    /// @param den  Denominator
    /// @throws no_rational_error   If the denominator is zero.
    template<class T, class U,
             typename
             = notf::enable_if_t<notf::conjunction<std::is_integral<T>, std::is_convertible<U, Integer>>::value>>
    constexpr Rational(const T& num, const U& den) : m_num(static_cast<Integer>(num)), m_den(static_cast<Integer>(den))
    {
        _normalize();
    }

    /// Copy constructor.
    template<class T>
    constexpr Rational(const Rational<T>& other)
        : m_num(static_cast<Integer>(other.m_num)), m_den(static_cast<Integer>(other.m_den))
    {}

    /// Assignment operator.
    template<class T>
    constexpr Rational& operator=(const Rational<T>& other)
    {
        m_num = static_cast<Integer>(other.m_num);
        m_den = static_cast<Integer>(other.m_den);
        return *this;
    }

    /// Numerator of the fraction (above the line).
    constexpr Integer get_numerator() const { return m_num; }

    /// Denominator of the fraction (under the line).
    constexpr Integer get_denominator() const { return m_den; }

    /// Returns the corresponding real value to this fraction.
    template<class T = float, typename = notf::enable_if_t<std::is_floating_point<T>::value>>
    constexpr T as_real() const
    {
        return static_cast<T>(m_num) / static_cast<T>(m_den);
    }

    /// Rational addition.
    /// @param r    Other rational number to add to this fraction.
    constexpr Rational& operator+=(const Rational& r)
    {
        // This calculation avoids overflow, and minimises the number of expensive calculations.
        // Thanks to Nickolay Mladenov for this algorithm.
        //
        // Proof:
        // We have to compute a/b + c/d, where gcd(a,b)=1 and gcd(b,c)=1.
        // Let g = gcd(b,d), and b = b1*g, d=d1*g. Then gcd(b1,d1)=1
        //
        // The result is (a*d1 + c*b1) / (b1*d1*g).
        // Now we have to normalize this ratio.
        // Let's assume h | gcd((a*d1 + c*b1), (b1*d1*g)), and h > 1
        // If h | b1 then gcd(h,d1)=1 and hence h|(a*d1+c*b1) => h|a.
        // But since gcd(a,b1)=1 we have h=1.
        // Similarly h|d1 leads to h=1.
        // So we have that h | gcd((a*d1 + c*b1) , (b1*d1*g)) => h|g
        // Finally we have gcd((a*d1 + c*b1), (b1*d1*g)) = gcd((a*d1 + c*b1), g)
        // Which proves that instead of normalizing the result, it is better to
        // divide num and den by gcd((a*d1 + c*b1), g)
        Integer gcd = notf::gcd(m_den, r.m_den);
        m_den /= gcd; // = b1 from the calculations above
        m_num = m_num * (r.m_den / gcd) + r.m_num * m_den;

        gcd = notf::gcd(m_num, gcd);
        m_num /= gcd;
        m_den *= r.m_den / gcd;
        return *this;
    }

    /// Rational subtraction.
    /// @param r    Other rational number to subtract from this fraction.
    constexpr Rational& operator-=(const Rational& r)
    {
        // This calculation avoids overflow, and minimises the number of expensive
        // calculations. It corresponds exactly to the += case above
        Integer gcd = notf::gcd(m_den, r.m_den);
        m_den /= gcd;
        m_num = m_num * (r.m_den / gcd) - r.m_num * m_den;

        gcd = notf::gcd(m_num, gcd);
        m_num /= gcd;
        m_den *= r.m_den / gcd;
        return *this;
    }

    /// Rational multiplication.
    /// @param r    Other rational number to multiply with this fraction.
    constexpr Rational& operator*=(const Rational& r)
    {
        // avoid overflow and preserve normalization
        Integer gcd1 = notf::gcd(m_num, r.m_den);
        Integer gcd2 = notf::gcd(r.m_num, m_den);
        m_num = (m_num / gcd1) * (r.m_num / gcd2);
        m_den = (m_den / gcd2) * (r.m_den / gcd1);
        return *this;
    }

    /// Rational division.
    /// @param r    Other rational number to divide this fraction by.
    constexpr Rational& operator/=(const Rational& r)
    {
        if (r.m_num == 0) {
            notf_throw(no_rational_error, "Cannot divide by zero");
        }
        if (m_num == 0) {
            return *this;
        }

        // avoid overflow and preserve normalization
        Integer gcd1 = notf::gcd(m_num, r.m_num);
        Integer gcd2 = notf::gcd(r.m_den, m_den);
        m_num = (m_num / gcd1) * (r.m_den / gcd2);
        m_den = (m_den / gcd2) * (r.m_num / gcd1);

        // ensure that the denominator is positive
        if (m_den < 0) {
            m_num = -m_num;
            m_den = -m_den;
        }
        return *this;
    }

    /// Scalar addition.
    /// @param i    Scalar to add to the fraction.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator+=(const T& i)
    {
        m_num += i * m_den;
        return *this;
    }

    /// Scalar subtraction.
    /// @param i    Scalar to subtract from the fraction.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator-=(const T& i)
    {
        m_num -= i * m_den;
        return *this;
    }

    /// Scalar multiplication.
    /// @param i    Scalar to multiply the fraction with.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator*=(const T& i)
    {
        // avoid overflow and preserve normalization
        const Integer gcd = notf::gcd(static_cast<Integer>(i), m_den);
        m_num *= i / gcd;
        m_den /= gcd;
        return *this;
    }

    /// Scalar division.
    /// @param i    Scalar to divide the fraction by.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator/=(const T& i)
    {
        if (i == 0) {
            notf_throw(no_rational_error, "Cannot divide by zero");
        }
        if (m_num == 0) {
            return *this;
        }

        // avoid overflow and preserve normalization
        const Integer gcd = notf::gcd(m_num, static_cast<Integer>(i));
        m_num /= gcd;
        m_den *= i / gcd;

        // ensure that the denominator is positive
        if (m_den < 0) {
            m_num = -m_num;
            m_den = -m_den;
        }
        return *this;
    }

    /// Checks if this Rational is zero.
    explicit constexpr operator bool() const { return m_num != 0; }

    /// Equality operator.
    /// @param other    Value to test against.
    constexpr bool operator==(const Rational& other) const { return (m_num == other.m_num) && (m_den == other.m_den); }

    /// Scalar equality operator.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator==(const T& i) const
    {
        return (m_den == 1) && (m_num == static_cast<Integer>(i));
    }

    /// Lesser-than operator.
    constexpr bool operator<(const Rational& r) const
    {
        // boost has a whole song and dance here ... I suppose this might work as well
        return as_real<double>() < r.as_real<double>();
    }

    /// Scalar lesser-than operator.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator<(const T& i) const
    {
        // break value into mixed-fraction form, with always-nonnegative remainder
        NOTF_ASSERT(m_den > 0);
        Integer quotient = m_num / m_den;
        Integer remainder = m_num % m_den;
        while (remainder < 0) {
            remainder += m_den;
            --quotient;
        }

        // Compare with just the quotient, since the remainder always bumps the value up.
        // Since q = floor(n/d), and if n/d < i then q < i,
        //                           if n/d == i then q == i
        //                           if n/d == i + r/d then q == i
        //                       and if n/d >= i + 1 then q >= i + 1 > i
        // therefore n/d < i iff q < i.
        return quotient < i;
    }

    /// Lesser-or-equal operator
    constexpr bool operator<=(const Rational& r) const { return operator==(r) || operator<(r); }

    /// Scalar lesser-or-equal operator
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator<=(const T& i) const
    {
        return operator==(i) || operator<(i);
    }

    /// Greater-than operator.
    constexpr bool operator>(const Rational& r) const { return operator==(r) ? false : !operator<(r); }

    /// Scalar greater-than operator.
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator>(const T& i) const
    {
        return operator==(i) ? false : !operator<(i);
    }

    /// Greater-or-equal operator
    constexpr bool operator>=(const Rational& r) const { return operator==(r) || operator>(r); }

    /// Scalar greater-or-equal operator
    template<class T, typename = notf::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator>=(const T& i) const
    {
        return operator==(i) || operator>(i);
    }

private:
    /// Normalizes the fraction.
    void _normalize()
    {
        if (m_den == 0) {
            notf_throw(no_rational_error, "{}/{} is not a valid rational number", m_num, m_den);
        }

        // normal zero
        if (m_num == 0) {
            m_den = 1;
            return;
        }

        // normalize
        const Integer gcd = notf::gcd(m_num, m_den);
        m_num /= gcd;
        m_den /= gcd;

        // ensure that the denominator is positive
        if (m_den < 0) {
            m_num = -m_num;
            m_den = -m_den;
        }
    }

    // fields ------------------------------------------------------------------------------------------------------- //
private:
    /// Numerator of the fraction (above the line).
    Integer m_num = 0;

    /// Denominator of the fraction (under the line).
    Integer m_den = 1;
};

/// Unary plus operator for Rationals.
template<class Integer>
inline constexpr Rational<Integer> operator+(const Rational<Integer>& r)
{
    return r;
}

/// Unary minus operator for Rationals.
template<class Integer>
inline constexpr Rational<Integer> operator-(const Rational<Integer>& r)
{
    return {static_cast<Integer>(-r.get_numerator()), r.get_denominator()};
}

/// Rational scalar addition.
template<class T, class Integer>
inline constexpr Rational<T> operator+(const Rational<T>& r, const Integer& i)
{
    return Rational<T>(r) += i;
}

/// Reverse rational scalar addition.
template<class Integer, class T>
inline constexpr Rational<Integer> operator+(const Integer& i, const Rational<T>& r)
{
    return Rational<Integer>(i) += r;
}

/// Rational scalar subtraction.
template<class T, class Integer>
inline constexpr Rational<T> operator-(const Rational<T>& r, const Integer& i)
{
    return Rational<T>(r) -= i;
}

/// Reverse rational scalar subtraction.
template<class Integer, class T>
inline constexpr Rational<Integer> operator-(const Integer& i, const Rational<T>& r)
{
    return Rational<Integer>(i) -= r;
}

/// Rational scalar multiplication.
template<class T, class Integer>
inline constexpr Rational<T> operator*(const Rational<T>& r, const Integer& i)
{
    return Rational<T>(r) *= i;
}

/// Reverse rational scalar multiplication.
template<class Integer, class T>
inline constexpr Rational<Integer> operator*(const Integer& i, const Rational<T>& r)
{
    return Rational<Integer>(i) *= r;
}

/// Rational scalar division.
template<class T, class Integer>
inline constexpr Rational<T> operator/(const Rational<T>& r, const Integer& i)
{
    return Rational<T>(r) /= i;
}

/// Scalar/Rational division.
template<class Integer, class T>
inline constexpr Rational<Integer> operator/(const Integer& i, const Rational<T>& r)
{
    return Rational<Integer>(i) /= r;
}

/// Scalar/Rational lesser-than operator.
template<class Integer, class T>
inline constexpr bool operator<(const Integer& i, const Rational<T>& r)
{
    return r > i;
}

/// Scalar/Rational greater-than operator.
template<class Integer, class T>
inline constexpr bool operator>(const Integer& i, const Rational<T>& r)
{
    return r < i;
}

/// Scalar/Rational lesser-or-equal operator.
template<class T, class Integer>
inline constexpr bool operator<=(const Integer& i, const Rational<T>& r)
{
    return !(r > i);
}

/// Scalar/Rational greater-or-equal operator.
template<class T, class Integer>
inline constexpr bool operator>=(const Integer& i, const Rational<T>& r)
{
    return !(r < i);
}

/// Scalar/Rational equality operator.
template<class T, class Integer>
inline constexpr bool operator==(const Integer& i, const Rational<T>& r)
{
    return r == i;
}

/// Scalar/Rational inequality operator.
template<class T, class Integer>
inline constexpr bool operator!=(const Integer& i, const Rational<T>& r)
{
    return !(r == i);
}

NOTF_CLOSE_NAMESPACE
