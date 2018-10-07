
#include <ostream>

#if __has_builtin(__builtin_launder)
#define _GLIBCXX_HAVE_BUILTIN_LAUNDER 1
#endif
#include <new>

#include "notf/common/msgpack.hpp"

NOTF_OPEN_NAMESPACE

template<typename T>
void write_data(const T& value, std::ostream& os)
{
    std::launder(reinterpret_cast<T*>(&value));
}

// msgpack value ==================================================================================================== //

class MsgPack {};

const MsgPack MsgPackValue::s_empty_pack = {};

NOTF_CLOSE_NAMESPACE
