#pragma once

/** Turns anything into a const reference of itself.
 * Useful for when you want to make sure to call the const overload of a method, for example.
 */
template<typename T>
const T& make_const(T& ref) {
	return ref;
}
