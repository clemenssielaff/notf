#include "common/signal.hpp"

namespace notf {

receive_signals::~receive_signals() { disconnect_all_connections(); }

} // namespace notf
