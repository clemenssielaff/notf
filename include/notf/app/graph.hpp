#pragma once

#include "notf/meta/log.hpp"

NOTF_OPEN_NAMESPACE

// the graph ======================================================================================================== //

class TheGraph {

    // methods ------------------------------------------------------------------------------------------------------ //
private:
    /// Default constructor.
    TheGraph() = default;

public:
    NOTF_NO_COPY_OR_ASSIGN(TheGraph);

    static TheGraph& get()
    {
        static TheGraph instance;
        return instance;
    }

    static bool is_frozen() noexcept { return s_is_frozen; }
    static bool is_frozen_by(std::thread::id /*thread_id*/) noexcept { return s_is_frozen; }

private:
    static inline bool s_is_frozen = false;
};

NOTF_CLOSE_NAMESPACE
