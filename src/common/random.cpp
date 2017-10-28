#include "common/random.hpp"

namespace notf {

decltype(randutils::default_rng()) & get_random_engine() {
	static auto random_engine = randutils::default_rng();
	return random_engine;
}

std::string random_string(const size_t length, const bool lowercase, const bool uppercase, const bool digits) {
	if (lowercase) {
		if (uppercase) {
			if (digits) {
				static const std::string pool = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
				return random_string(length, pool);
			}
			else {
				static const std::string pool = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
				return random_string(length, pool);
			}
		}
		else { // no uppercase
			if (digits) {
				static const std::string pool = "abcdefghijklmnopqrstuvwxyz0123456789";
				return random_string(length, pool);
			}
			else {
				static const std::string pool = "abcdefghijklmnopqrstuvwxyz";
				return random_string(length, pool);
			}
		}
	}
	else { // no lowercase
		if (uppercase) {
			if (digits) {
				static const std::string pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
				return random_string(length, pool);
			}
			else {
				static const std::string pool = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
				return random_string(length, pool);
			}
		}
		else { // no uppercase
			if (digits) {
				static const std::string pool = "0123456789";
				return random_string(length, pool);
			}
			else {
				static const std::string pool = "";
				return random_string(length, pool);
			}
		}
	}
}

std::string random_string(const size_t length, const std::string& pool) {
	if (pool.empty()) {
		return {};
	}
	std::string result(length, pool[0]);
	auto random_engine = get_random_engine();
	for (size_t i = 0; i < length; ++i) {
		result[i] = *random_engine.choose(std::begin(pool), std::end(pool));
	}
	return result;
}

} // namespace notf
