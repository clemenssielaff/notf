#pragma once

// This code is bascially a notf-ed version of Boost rational.hpp, which is why I include the original license text of
// that file here:
//
//  (C) Copyright Paul Moore 1999. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or
//  implied warranty, and with no claim as to its suitability for any purpose.

#include "notf/meta/hash.hpp"
#include "notf/meta/integer.hpp"

NOTF_OPEN_NAMESPACE

// rational ========================================================================================================= //

/// Exception thrown when you try to constuct an invalid rational number.
NOTF_EXCEPTION_TYPE(BadRationalError);

namespace detail {

/// A rational number consisting of an integer fraction.
template<class Integer, class = std::enable_if_t<std::is_integral<Integer>::value>>
class Rational {

    // types ----------------------------------------------------------------------------------- //
public:
    /// Element type.
    using element_t = Integer;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Default constructor, does not initializel.
    Rational() = default;

    /// Value constructor.
    /// @param num  Numerator, Denominator defaults to one.
    constexpr Rational(const element_t num) : m_num(num), m_den(1) {}

    /// Value constructor.
    /// @param num  Numerator.
    /// @param den  Denominator
    /// @throws BadRationalError    If the denominator is zero.
    constexpr Rational(const element_t& num, const element_t& den) : m_num(num), m_den(den) { _normalize(); }

    /// Copy constructor.
    template<class T>
    constexpr Rational(const Rational<T>& other)
        : m_num(static_cast<element_t>(other.m_num)), m_den(static_cast<element_t>(other.m_den))
    {}

    /// Name of this Rational type.
    static constexpr const char* get_name()
    {
        if constexpr (std::is_same_v<Integer, int>) {
            return "Ratioi";
        } else if constexpr (std::is_same_v<Integer, short>) {
            return "Ratios";
        }
    }

    /// Assignment operator.
    template<class T>
    constexpr Rational& operator=(const Rational<T>& other) noexcept
    {
        m_num = static_cast<element_t>(other.m_num);
        m_den = static_cast<element_t>(other.m_den);
        return *this;
    }

    /// Explicitly creates and returns a zero Rational.
    constexpr static Rational zero() { return {0, 0}; }

    /// Numerator of the fraction (above the line).
    constexpr element_t& num() noexcept { return m_num; }
    constexpr const element_t& num() const noexcept { return m_num; }

    /// Denominator of the fraction (below the line).
    constexpr element_t& den() noexcept { return m_den; }
    constexpr const element_t& den() const noexcept { return m_den; }

    /// Returns the corresponding real value to this fraction.
    /// A rational number with a denominator of zero produces a zero result.
    template<class T = float, class = std::enable_if_t<std::is_floating_point<T>::value>>
    constexpr T as_real() const noexcept
    {
        if (m_den == 0) {
            return 0;
        } else {
            return static_cast<T>(m_num) / static_cast<T>(m_den);
        }
    }

    /// Checks if this Rational is zero.
    constexpr bool is_zero() const noexcept { return m_num == 0; }

    /// Sets the numerator (above the line).
    /// @param i    New numerator.
    constexpr void set_numerator(element_t i)
    {
        m_num = i;
        _normalize();
    }

    /// Sets the denominator (above the line).
    /// @param i    New denominator.
    /// @throws BadRationalError    If the denominator is zero.
    constexpr void set_denominator(element_t i)
    {
        m_den = i;
        _normalize();
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
        element_t gcd = notf::gcd(m_den, r.m_den);
        m_den /= gcd; // = b1 from the calculations above
        m_num = m_num * (r.m_den / gcd) + r.m_num * m_den;

        gcd = notf::gcd(m_num, gcd);
        m_num /= gcd;
        m_den *= r.m_den / gcd;
        return *this;
    }
    constexpr Rational operator+(const Rational& other) const& { return Rational(this) += other; }
    constexpr Rational& operator+(const Rational& other) && { return *this += other; }

    /// Rational subtraction.
    /// @param r    Other rational number to subtract from this fraction.
    constexpr Rational& operator-=(const Rational& other)
    {
        // This calculation avoids overflow, and minimises the number of expensive
        // calculations. It corresponds exactly to the += case above
        element_t gcd = notf::gcd(m_den, other.m_den);
        m_den /= gcd;
        m_num = m_num * (other.m_den / gcd) - other.m_num * m_den;

        gcd = notf::gcd(m_num, gcd);
        m_num /= gcd;
        m_den *= other.m_den / gcd;
        return *this;
    }
    constexpr Rational operator-(const Rational& other) const& { return Rational(this) -= other; }
    constexpr Rational& operator-(const Rational& other) && { return *this -= other; }

    /// Rational multiplication.
    /// @param r    Other rational number to multiply with this fraction.
    constexpr Rational& operator*=(const Rational& other)
    {
        // avoid overflow and preserve normalization
        element_t gcd1 = notf::gcd(m_num, other.m_den);
        element_t gcd2 = notf::gcd(other.m_num, m_den);
        m_num = (m_num / gcd1) * (other.m_num / gcd2);
        m_den = (m_den / gcd2) * (other.m_den / gcd1);
        return *this;
    }
    constexpr Rational operator*(const Rational& other) const& { return Rational(this) *= other; }
    constexpr Rational& operator*(const Rational& other) && { return *this *= other; }

    /// Rational division.
    /// @param r    Other rational number to divide this fraction by.
    constexpr Rational& operator/=(const Rational& other)
    {
        if (other.is_zero()) { NOTF_THROW(BadRationalError, "Cannot divide by zero"); }
        if (is_zero()) { return *this; }

        // avoid overflow and preserve normalization
        element_t gcd1 = notf::gcd(m_num, other.m_num);
        element_t gcd2 = notf::gcd(other.m_den, m_den);
        m_num = (m_num / gcd1) * (other.m_den / gcd2);
        m_den = (m_den / gcd2) * (other.m_num / gcd1);

        // ensure that the denominator is positive
        if (m_den < 0) {
            m_num = -m_num;
            m_den = -m_den;
        }
        return *this;
    }
    constexpr Rational operator/(const Rational& other) const& { return Rational(this) /= other; }
    constexpr Rational& operator/(const Rational& other) && { return *this /= other; }

    /// Scalar addition.
    /// @param i    Scalar to add to the fraction.
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator+=(const T& i) noexcept
    {
        m_num += i * m_den;
        return *this;
    }

    /// Scalar subtraction.
    /// @param i    Scalar to subtract from the fraction.
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator-=(const T& i) noexcept
    {
        m_num -= i * m_den;
        return *this;
    }

    /// Scalar multiplication.
    /// @param i    Scalar to multiply the fraction with.
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator*=(const T& i)
    {
        // avoid overflow and preserve normalization
        const element_t gcd = notf::gcd(static_cast<element_t>(i), m_den);
        m_num *= i / gcd;
        m_den /= gcd;
        return *this;
    }

    /// Scalar division.
    /// @param i    Scalar to divide the fraction by.
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr Rational& operator/=(const T& i)
    {
        if (i == 0) { NOTF_THROW(BadRationalError, "Cannot divide by zero"); }
        if (is_zero()) { return *this; }

        // avoid overflow and preserve normalization
        const element_t gcd = notf::gcd(m_num, static_cast<element_t>(i));
        m_num /= gcd;
        m_den *= i / gcd;

        // ensure that the denominator is positive
        if (m_den < 0) {
            m_num = -m_num;
            m_den = -m_den;
        }
        return *this;
    }

    /// Unary plus operator for Ratios.
    constexpr Rational operator+() const noexcept { return *this; }

    /// Unary minus operator for Ratios.
    constexpr Rational operator-() const noexcept { return {-m_num, m_den}; }

    // mixed arithmetic -------------------------------------------------------

    /// Rational scalar addition.
    friend constexpr Rational operator+(Rational r, const element_t i) noexcept { return r += i; }
    friend constexpr Rational operator+(const element_t i, Rational r) noexcept { return r += i; }

    /// Rational scalar subtraction.
    friend constexpr Rational operator-(Rational r, const element_t i) noexcept { return r -= i; }
    friend constexpr Rational operator-(const element_t i, const Rational& r) { return Rational(i) -= r; }

    /// Rational scalar multiplication.
    friend constexpr Rational operator*(Rational r, const Integer i) { return r *= i; }
    friend constexpr Rational operator*(const Integer i, Rational r) { return r *= i; }

    /// Rational scalar division.
    friend constexpr Rational operator/(Rational r, const Integer i) { return r /= i; }
    friend constexpr Rational operator/(const Integer i, const Rational& r) { return Rational(i) /= r; }

    // comparison -------------------------------------------------------------

    /// Checks if this Rational is zero.
    explicit constexpr operator bool() const { return m_num != 0; }

    /// Equality operator.
    /// @param other    Value to test against.
    constexpr bool operator==(const Rational& other) const { return (m_num == other.m_num) && (m_den == other.m_den); }

    /// Scalar equality operator.
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator==(const T& i) const
    {
        return (m_den == 1) && (m_num == static_cast<element_t>(i));
    }

    /// Lesser-than operator.
    constexpr bool operator<(const Rational& r) const
    {
        // boost has a whole song and dance here ... I suppose this might work as well
        return as_real<double>() < r.as_real<double>();
    }

    /// Scalar lesser-than operator.
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator<(const T& i) const
    {
        // break value into mixed-fraction form, with always-nonnegative remainder
        NOTF_ASSERT(m_den > 0);
        element_t quotient = m_num / m_den;
        element_t remainder = m_num % m_den;
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
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator<=(const T& i) const
    {
        return operator==(i) || operator<(i);
    }

    /// Greater-than operator.
    constexpr bool operator>(const Rational& r) const { return operator==(r) ? false : !operator<(r); }

    /// Scalar greater-than operator.
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator>(const T& i) const
    {
        return operator==(i) ? false : !operator<(i);
    }

    /// Greater-or-equal operator
    constexpr bool operator>=(const Rational& r) const { return operator==(r) || operator>(r); }

    /// Scalar greater-or-equal operator
    template<class T, class = std::enable_if_t<std::is_integral<T>::value>>
    constexpr bool operator>=(const T& i) const
    {
        return operator==(i) || operator>(i);
    }

    /// Scalar/Rational lesser-than operator.
    friend constexpr bool operator<(const Integer i, const Rational& r) noexcept { return r > i; }

    /// Scalar/Rational greater-than operator.
    friend constexpr bool operator>(const Integer i, const Rational& r) noexcept { return r < i; }

    /// Scalar/Rational lesser-or-equal operator.
    friend constexpr bool operator<=(const Integer i, const Rational& r) noexcept { return !(r > i); }

    /// Scalar/Rational greater-or-equal operator.
    friend constexpr bool operator>=(const Integer i, const Rational& r) noexcept { return !(r < i); }

    /// Scalar/Rational equality operator.
    friend constexpr bool operator==(const Integer i, const Rational& r) noexcept { return r == i; }

    /// Scalar/Rational inequality operator.
    friend constexpr bool operator!=(const Integer i, const Rational& r) noexcept { return !(r == i); }

private:
    /// Normalizes the fraction.
    constexpr void _normalize()
    {
        if (m_den == 0) { NOTF_THROW(BadRationalError, "{}/{} is not a valid rational number", m_num, m_den); }

        // normal zero
        if (is_zero()) {
            m_den = 1;
            return;
        }

        // normalize
        const element_t gcd = notf::gcd(m_num, m_den);
        m_num /= gcd;
        m_den /= gcd;

        // ensure that the denominator is positive
        if (m_den < 0) {
            m_num = -m_num;
            m_den = -m_den;
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Numerator of the fraction (above the line).
    element_t m_num;

    /// Denominator of the fraction (below the line).
    element_t m_den;
};

} // namespace detail

using Ratioi = detail::Rational<int>;
using Ratios = detail::Rational<short>;

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

namespace std {

/// std::hash specialization for notf::Rational<T>.
template<class Integer>
struct hash<notf::detail::Rational<Integer>> {
    size_t operator()(const notf::detail::Rational<Integer>& rational) const
    {
        return notf::hash(notf::to_number(notf::detail::HashID::RATIONAL), rational.num(), rational.den());
    }
};

} // namespace std

// formatting ======================================================================================================= //

namespace fmt {

template<class Integer>
struct formatter<notf::detail::Rational<Integer>> {
    using type = notf::detail::Rational<Integer>;

    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const type& rational, FormatContext& ctx)
    {
        return format_to(ctx.begin(), "{}({}/{})", type::get_name(), rational.num(), rational.den());
    }
};

} // namespace fmt

/// Prints the contents of a vector into a std::ostream.
/// @param os   Output stream, implicitly passed with the << operator.
/// @param vec  Vector to print.
/// @return Output stream for further output.
template<class Integer>
std::ostream& operator<<(std::ostream& out, const notf::detail::Rational<Integer>& rational)
{
    return out << fmt::format("{}", rational);
}

// compile time tests =============================================================================================== //

static_assert(std::is_pod_v<notf::Ratioi>);
static_assert(std::is_pod_v<notf::Ratios>);

static_assert(sizeof(notf::Ratioi) == sizeof(int) * 2);
static_assert(sizeof(notf::Ratios) == sizeof(short) * 2);
