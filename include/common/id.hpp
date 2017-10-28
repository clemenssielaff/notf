#pragma once

#include <iostream>
#include <type_traits>

namespace notf {

/** Strongly typed integral identifier.
 * Is useful if you have multiple types of identifiers that are all the same underlying type and don't want them to be
 * assigneable / comparable.
 */
template<typename Type, typename underlying_type, typename = std::enable_if_t<std::is_integral<underlying_type>::value>>
struct Id {
	using type_t       = Type;
	using underlying_t = underlying_type;

	/** Invalid Id. */
	static const underlying_type INVALID = 0;

	/** Default constructor, produces an invalid ID. */
	Id() : id(INVALID) {}

	/** Value constructor. */
	Id(underlying_type id) : id(id) {}

	/** Equal operator. */
	template<typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
	bool operator==(const Other& rhs) const {
		return id == rhs.id;
	}

	bool operator==(const underlying_t& rhs) const { return id == rhs; }

	/** Not-equal operator. */
	template<typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
	bool operator!=(const Other& rhs) const {
		return id != rhs.id;
	}

	bool operator!=(const underlying_t& rhs) const { return id != rhs; }

	/** Lesser-then operator. */
	template<typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
	bool operator<(const Other& rhs) const {
		return id < rhs.id;
	}

	bool operator<(const underlying_t& rhs) const { return id < rhs; }

	/** Lesser-or-equal-then operator. */
	template<typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
	bool operator<=(const Other& rhs) const {
		return id <= rhs.id;
	}

	bool operator<=(const underlying_t& rhs) const { return id <= rhs; }

	/** Greater-then operator. */
	template<typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
	bool operator>(const Other& rhs) const {
		return id > rhs.id;
	}

	bool operator>(const underlying_t& rhs) const { return id > rhs; }

	/** Greater-or-equal-then operator. */
	template<typename Other, typename = std::enable_if_t<std::is_same<typename Other::type_t, type_t>::value>>
	bool operator>=(const Other& rhs) const {
		return id >= rhs.id;
	}

	bool operator>=(const underlying_t& rhs) const { return id >= rhs; }

	/** Checks if this Id is valid or not. */
	explicit operator bool() const { return id != INVALID; }

	/** Cast back to the underlying type, must be explicit to avoid comparison between different Id types. */
	explicit operator underlying_type() const { return id; }

	/** Identifier value of this instance. */
	const underlying_type id;
};

/** Prints the contents of an Id into a std::ostream.
 * @param os   Output stream, implicitly passed with the << operator.
 * @param id   Id to print.
 * @return Output stream for further output.
 */
template<typename Type, typename underlying_type>
std::ostream& operator<<(std::ostream& out, const Id<Type, underlying_type>& id) {
	return out << id.id;
}

} // namespace notf
