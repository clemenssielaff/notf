#pragma once

#include <string>

namespace Catch {

inline std::string toString(int v) { return std::to_string(v); }
inline std::string toString(uint v) { return std::to_string(v); }
inline std::string toString(float v) { return std::to_string(v); }
inline std::string toString(double v) { return std::to_string(v); }
inline std::string toString(bool v) { return std::to_string(v); }

}
