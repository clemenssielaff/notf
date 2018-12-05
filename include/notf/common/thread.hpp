#pragma once

#include <array>
#include <atomic>
#include <functional>
#include <thread>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"
#include "notf/meta/system.hpp"
#include "notf/meta/types.hpp"

#include "notf/common/fwd.hpp"

NOTF_OPEN_NAMESPACE

// this_thread ====================================================================================================== //

namespace this_thread {
using namespace ::std::this_thread;

namespace detail {

/// Id of the main thread.
static const std::thread::id main_thread_id = std::this_thread::get_id();

struct ThreadInfo {

    friend Accessor<ThreadInfo, ::notf::Thread>;

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
        EVENT,         ///< The single event handler thread, is the UI thread for most of the Application's lifetime.
        RENDER,        ///< The single render thread, used to stage data for the render pipeline.
        TIMER_POOL,    ///< Single TimerPool thread used to wait on the next Timer.
        WORKER,        ///< One of n worker threads used for all kind of blocking or long-running functions.
        __LAST,
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// The kind of this thread.
    static auto get_kind() noexcept { return s_kind; }

private:
    /// How many Threads are allowed for each kind?
    constexpr static size_t _get_kind_limit(Kind kind) noexcept {
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

/// Tests if this is the main thread.
inline bool is_main_thread() { return std::this_thread::get_id() == detail::main_thread_id; }

} // namespace this_thread

template<>
class Accessor<this_thread::detail::ThreadInfo, Thread> {
    friend Thread;

    /// Registers a new thread of a given kind.
    /// @param kind         Kind of Thread to add.
    /// @throws ThreadError If the maximum number of Threads has been exceeded for the given kind.
    static void add_one_of_kind(const this_thread::detail::ThreadInfo::Kind kind) {
        using ThreadInfo = this_thread::detail::ThreadInfo;
        const auto previous = ThreadInfo::s_kind_counter[to_number(kind)].fetch_add(1);
        if (previous >= ThreadInfo::_get_kind_limit(kind)) {
            remove_one_of_kind(kind);
            NOTF_THROW(ThreadError, "Cannot create more than one Thread of Kind {}", to_number(kind));
        }
        ThreadInfo::s_kind = kind;
    }

    static void remove_one_of_kind(const this_thread::detail::ThreadInfo::Kind kind) {
        using ThreadInfo = this_thread::detail::ThreadInfo;
        NOTF_UNUSED const auto previous = ThreadInfo::s_kind_counter[to_number(kind)].fetch_sub(1);
        NOTF_ASSERT(previous != 0);
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

private:
    struct KindCounterGuard {
        using ThreadInfoAccess = this_thread::detail::ThreadInfo::AccessFor<Thread>;
        KindCounterGuard(Kind kind) : m_kind(kind) { ThreadInfoAccess::add_one_of_kind(m_kind); }
        ~KindCounterGuard() { ThreadInfoAccess::remove_one_of_kind(m_kind); }

    private:
        const Kind m_kind;
    };

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
    Thread& operator=(Thread&& other) {
        join();
        m_thread = std::move(other.m_thread);
        m_exception = std::move(other.m_exception);
        m_kind = other.m_kind;
        return *this;
    }

    /// Destructor.
    /// Blocks until the system thread has joined.
    /// Any stored exception is silently dropped.
    ~Thread() { join(); }

    /// Run a given function on this thread.
    /// Any stored exception is silently dropped.
    /// @throws ThreadError If this thread is already executing a function.
    template<class Function>
    void run(Function&& function) {
        if (is_running()) {
            NOTF_THROW(ThreadError, "Thread is already running a function.\n"
                                    "If you need this particular thread to run, join it first");
        }

        m_exception = {};
        m_thread = std::thread([this, function = std::forward<Function>(function)]() mutable {
            try {
                NOTF_GUARD(KindCounterGuard(m_kind));
                std::invoke(function);
            }
            catch (...) {
                m_exception = std::current_exception();
            }
        });
    }

    /// Kind of this thread.
    Kind get_kind() const noexcept { return m_kind; }

    /// Tests if this Thread instance is currently running.
    bool is_running() const noexcept { return m_thread.joinable(); }

    /// Tests if an exception has occurred during the last run.
    bool has_exception() const noexcept { return m_exception != nullptr; }

    /// Rethrows and clears the stored exception from the last run, if there is one.
    void rethrow() {
        if (m_exception) {
            std::exception_ptr exception;
            std::swap(exception, m_exception);
            std::rethrow_exception(std::move(exception));
        }
    }

    /// Blocks until the system thread has joined.
    void join() {
        if (is_running()) { m_thread.join(); }
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// System thread to manage.
    std::thread m_thread;

    /// Exception thrown during the last run of this Thread.
    std::exception_ptr m_exception;

    /// Kind of this thread.
    Kind m_kind = Kind::UNDEFINED;
};

// thread id ======================================================================================================== //

/// Converts a std::thread::id to its corresponding unsigned integer type.
inline auto to_number(std::thread::id id) noexcept { return *std::launder(reinterpret_cast<Thread::NumericId*>(&id)); }

NOTF_CLOSE_NAMESPACE
