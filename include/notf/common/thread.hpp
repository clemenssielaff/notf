#pragma once

#include <array>
#include <atomic>
#include <thread>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/system.hpp"
#include "notf/meta/types.hpp"

#include "notf/common/common.hpp"

NOTF_OPEN_NAMESPACE

// this_thread ====================================================================================================== //

namespace this_thread {
using namespace ::std::this_thread;

namespace detail {
struct ThreadInfo {

    friend Accessor<ThreadInfo, ::NOTF_NAMESPACE_NAME::Thread>;

    // types ----------------------------------------------------------------------------------- //
public:
    /// Nested `AccessFor<T>` type.
    NOTF_ACCESS_TYPE(ThreadInfo);

    /// Different threads fulfill different application needs.
    /// In order to ensure that a thread behaves as we expect it to, every Thread can have a `Kind` that can be checked
    /// at runtime.
    enum class Kind : unsigned {
        UNDEFINED = 0, ///< The default, probably a thread not managed by notf.
        MAIN,          ///< The single main thread.
        EVENT,         ///< The single event handler thread, the only one allowed to modify the Graph.
        RENDER,        ///< The single render thread, used to stage data for the render pipeline.
        WORKER,        ///< One of n worker threads used for all kind of blocking or long-running functions.
        __LAST,
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// The kind of this thread.
    static auto get_kind() noexcept { return s_kind; }

private:
    /// How many Threads are allowed for each kind?
    constexpr static size_t _get_kind_limit(Kind kind) noexcept
    {
        switch (kind) {
        case Kind::MAIN:
        case Kind::EVENT:
        case Kind::RENDER:
            return 1;

        default:
            return std::numeric_limits<size_t>::max();
        }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The kind of this thread.
    /// Is only set once at the start of a Thread::run(), or not at all.
    inline static thread_local Kind s_kind = Kind::UNDEFINED;

    /// Counter for each Thread kind.
    /// Is used to enforce how many Threads there can be of any one type (for example, there will only ever be one main
    /// thread).
    inline static std::array<std::atomic_size_t, to_number(Kind::__LAST)> s_kind_counter;
};
} // namespace detail

/// The kind of this thread.
inline auto get_kind() { return detail::ThreadInfo::get_kind(); }

} // namespace this_thread

template<>
class Accessor<this_thread::detail::ThreadInfo, Thread> {
    friend Thread;

    /// Registers a new thread of a given kind.
    /// @param kind         Kind of Thread to add.
    /// @throws LogicError  If the maximum number of Threads has been exceeded for the given kind.
    static void add_one_of_kind(const this_thread::detail::ThreadInfo::Kind kind)
    {
        using ThreadInfo = this_thread::detail::ThreadInfo;
        const auto previous_count = ThreadInfo::s_kind_counter[to_number(kind)].fetch_add(1, std::memory_order_relaxed);
        if (previous_count >= ThreadInfo::_get_kind_limit(kind)) {
            remove_one_of_kind(kind);
            NOTF_THROW(LogicError, "Cannot create more than one Thread of Kind {}", to_number(kind));
        }
        ThreadInfo::s_kind = kind;
    }

    static void remove_one_of_kind(const this_thread::detail::ThreadInfo::Kind kind)
    {
        using ThreadInfo = this_thread::detail::ThreadInfo;
        const auto previous_count = ThreadInfo::s_kind_counter[to_number(kind)].fetch_sub(1, std::memory_order_relaxed);
        NOTF_ASSERT(previous_count != 0);
    }
};

// thread =========================================================================================================== //

/// Wraps and owns a thread that is automatically joined when destructed.
/// Adapted from "C++ Concurrency in Action: Practial Multithreading" Listing 2.6
class Thread {

    // types ----------------------------------------------------------------------------------- //
public:
    /// The Kind of this thread.
    using Kind = this_thread::detail::ThreadInfo::Kind;

    /// Unsigned integer able to represent a thread id.
    using NumericId = templated_unsigned_integer_t<bitsizeof<std::thread::id>()>;

    // methods --------------------------------------------------------------------------------- //
public:
    NOTF_NO_COPY_OR_ASSIGN(Thread);

    /// Constructor.
    /// @param kind     Kind of this thread.
    Thread(Kind kind = Kind::UNDEFINED) : m_kind(kind) {}

    /// Move Constructor.
    /// @param other    Thread to move from.
    Thread(Thread&& other) = default;

    /// Move Assignment.
    /// Blocks until the the current system thread is joined (if it is joinable).
    /// @param other    Thread to move from.
    Thread& operator=(Thread&& other)
    {
        if (m_thread.joinable()) { m_thread.join(); }
        m_thread = std::move(other.m_thread);
        m_kind = other.m_kind;
        return *this;
    }

    /// Destructor.
    /// Blocks until the system thread has joined.
    ~Thread()
    {
        if (m_thread.joinable()) { m_thread.join(); }
    }

    /// Run a given function on this thread.
    /// If another function is already running, this call will block until it has finished.
    template<class Function, class... Args>
    void run(Function&& function, Args&&... args)
    {
        if (m_thread.joinable()) { m_thread.join(); }
        m_thread = std::thread([&]() {
            using ThreadInfoAccess = this_thread::detail::ThreadInfo::AccessFor<Thread>;
            ThreadInfoAccess::add_one_of_kind(m_kind);
            std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
            ThreadInfoAccess::remove_one_of_kind(m_kind);
        });
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// System thread to manage.
    std::thread m_thread;

    /// Kind of this thread.
    Kind m_kind = Kind::UNDEFINED;
};

// thread id ======================================================================================================== //

/// Converts a std::thread::id to its corresponding unsigned integer type.
inline auto to_number(std::thread::id id) noexcept { return *std::launder(reinterpret_cast<Thread::NumericId*>(&id)); }

NOTF_CLOSE_NAMESPACE
