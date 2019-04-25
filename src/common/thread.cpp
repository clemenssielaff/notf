#include "notf/common/thread.hpp"

#include "notf/meta/log.hpp"

NOTF_OPEN_NAMESPACE

// thread =========================================================================================================== //

Thread::~Thread() {
    if (is_running()) {
        // The destructor is not the best place to join a thread as it may be called in the context of an exception
        // or whatever, where you don't want to wait for the thread to join (which might be never...).
        // Then again, it is a good idea to join the thread eventually and where if not here. So we'll do it, but still
        // give a warning to let you know that this should be fixed.
        NOTF_LOG_WARN("Joining Thread of kind \"{}\" in destructor.", get_kind_name(m_kind));
        join();
    }
}

NOTF_CLOSE_NAMESPACE
