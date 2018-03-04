#include "common/signal.hpp"

NOTF_OPEN_NAMESPACE

receive_signals::~receive_signals() { disconnect_all_connections(); }

NOTF_CLOSE_NAMESPACE
