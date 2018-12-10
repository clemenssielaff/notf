#pragma once

#include "notf/meta/hash.hpp"

NOTF_OPEN_NAMESPACE

// Delegate ========================================================================================================= //

template<class T>
class Delegate;

/// Delegate is a replacement for std::function.
/// Blatantly copied (with minor modifications) from https://codereview.stackexchange.com/q/14730
template<class Result, class... Args>
class Delegate<Result(Args...)> {

    friend struct std::hash<Delegate>;

    // types ----------------------------------------------------------------------------------- //
private:
    using stub_ptr_type = Result (*)(void*, Args&&...);

    using deleter_type = void (*)(void*);

    template<class C>
    using member_pair = std::pair<C* const, Result (C::*const)(Args...)>;

    template<class C>
    using const_member_pair = std::pair<C const* const, Result (C::*const)(Args...) const>;

    template<class>
    struct is_member_pair : std::false_type {};

    template<class C>
    struct is_member_pair<std::pair<C* const, Result (C::*const)(Args...)>> : std::true_type {};

    template<class>
    struct is_const_member_pair : std::false_type {};

    template<class C>
    struct is_const_member_pair<std::pair<C const* const, Result (C::*const)(Args...) const>> : std::true_type {};

    // methods --------------------------------------------------------------------------------- //
private:
    Delegate(void* const object, stub_ptr_type const method) noexcept : m_obj_ptr(object), m_stub_ptr(method) {}

public:
    Delegate() = default;

    Delegate(Delegate const&) = default;

    Delegate(Delegate&&) = default;

    Delegate(std::nullptr_t const) noexcept : Delegate() {}

    template<class C, class = std::enable_if_t<std::is_class_v<C>>>
    explicit Delegate(C const* const object) noexcept : m_obj_ptr(const_cast<C*>(object)) {}

    template<class C, class = std::enable_if_t<std::is_class_v<C>>>
    explicit Delegate(C const& object) noexcept : m_obj_ptr(const_cast<C*>(&object)) {}

    template<class C>
    Delegate(C* const object_ptr, Result (C::*const method_ptr)(Args...)) {
        *this = from(object_ptr, method_ptr);
    }

    template<class C>
    Delegate(C* const object_ptr, Result (C::*const method_ptr)(Args...) const) {
        *this = from(object_ptr, method_ptr);
    }

    template<class C>
    Delegate(C& object, Result (C::*const method_ptr)(Args...)) {
        *this = from(object, method_ptr);
    }

    template<class C>
    Delegate(C const& object, Result (C::*const method_ptr)(Args...) const) {
        *this = from(object, method_ptr);
    }

    template<class T, class functor_type = std::decay_t<T>,
             class = std::enable_if_t<!std::is_same_v<Delegate, functor_type>>>
    Delegate(T&& f)
        : m_storage(operator new(sizeof(std::decay_t<T>)), functor_deleter<std::decay_t<T>>)
        , m_storage_size(sizeof(std::decay_t<T>)) {
        new (m_storage.get()) functor_type(std::forward<T>(f));
        m_obj_ptr = m_storage.get();
        m_stub_ptr = functor_stub<functor_type>;
        m_deleter = deleter_stub<functor_type>;
    }

    Delegate& operator=(Delegate const&) = default;

    Delegate& operator=(Delegate&&) = default;

    template<class C>
    Delegate& operator=(Result (C::*const rhs)(Args...)) {
        return *this = from(static_cast<C*>(m_obj_ptr), rhs);
    }

    template<class C>
    Delegate& operator=(Result (C::*const rhs)(Args...) const) {
        return *this = from(static_cast<C const*>(m_obj_ptr), rhs);
    }

    template<class T, class functor_type = std::decay_t<T>,
             class = std::enable_if_t<!std::is_same_v<Delegate, functor_type>>>
    Delegate& operator=(T&& f) {
        if ((sizeof(functor_type) > m_storage_size) || !m_storage.unique()) {
            m_storage.reset(operator new(sizeof(functor_type)), functor_deleter<functor_type>);
            m_storage_size = sizeof(functor_type);
        } else {
            m_deleter(m_storage.get());
        }

        new (m_storage.get()) functor_type(std::forward<T>(f));
        m_obj_ptr = m_storage.get();
        m_stub_ptr = functor_stub<functor_type>;
        m_deleter = deleter_stub<functor_type>;
        return *this;
    }

    template<Result (*const function_ptr)(Args...)>
    static Delegate from() noexcept {
        return {nullptr, function_stub<function_ptr>};
    }

    template<class C, Result (C::*const method_ptr)(Args...)>
    static Delegate from(C* const object_ptr) noexcept {
        return {object_ptr, method_stub<C, method_ptr>};
    }

    template<class C, Result (C::*const method_ptr)(Args...) const>
    static Delegate from(C const* const object_ptr) noexcept {
        return {const_cast<C*>(object_ptr), const_method_stub<C, method_ptr>};
    }

    template<class C, Result (C::*const method_ptr)(Args...)>
    static Delegate from(C& object) noexcept {
        return {&object, method_stub<C, method_ptr>};
    }

    template<class C, Result (C::*const method_ptr)(Args...) const>
    static Delegate from(C const& object) noexcept {
        return {const_cast<C*>(&object), const_method_stub<C, method_ptr>};
    }

    template<class T>
    static Delegate from(T&& f) {
        return std::forward<T>(f);
    }

    static Delegate from(Result (*const function_ptr)(Args...)) { return function_ptr; }

    template<class C>
    static Delegate from(C* const object_ptr, Result (C::*const method_ptr)(Args...)) {
        return member_pair<C>(object_ptr, method_ptr);
    }

    template<class C>
    static Delegate from(C const* const object_ptr, Result (C::*const method_ptr)(Args...) const) {
        return const_member_pair<C>(object_ptr, method_ptr);
    }

    template<class C>
    static Delegate from(C& object, Result (C::*const method_ptr)(Args...)) {
        return member_pair<C>(&object, method_ptr);
    }

    template<class C>
    static Delegate from(C const& object, Result (C::*const method_ptr)(Args...) const) {
        return const_member_pair<C>(&object, method_ptr);
    }

    void reset() {
        m_stub_ptr = nullptr;
        m_storage.reset();
    }

    void reset_stub() noexcept { m_stub_ptr = nullptr; }

    void swap(Delegate& other) noexcept { std::swap(*this, other); }

    bool operator==(Delegate const& rhs) const noexcept {
        return (m_obj_ptr == rhs.m_obj_ptr) && (m_stub_ptr == rhs.m_stub_ptr);
    }

    bool operator!=(Delegate const& rhs) const noexcept { return !operator==(rhs); }

    bool operator<(Delegate const& rhs) const noexcept {
        return (m_obj_ptr < rhs.m_obj_ptr) || ((m_obj_ptr == rhs.m_obj_ptr) && (m_stub_ptr < rhs.m_stub_ptr));
    }

    bool operator==(std::nullptr_t const) const noexcept { return !m_stub_ptr; }

    bool operator!=(std::nullptr_t const) const noexcept { return m_stub_ptr; }

    explicit operator bool() const noexcept { return m_stub_ptr; }

    Result operator()(Args... args) const { return m_stub_ptr(m_obj_ptr, std::forward<Args>(args)...); }

private:
    template<class T>
    static void functor_deleter(void* const p) {
        static_cast<T*>(p)->~T();
        operator delete(p);
    }

    template<class T>
    static void deleter_stub(void* const p) {
        static_cast<T*>(p)->~T();
    }

    template<Result (*function_ptr)(Args...)>
    static Result function_stub(void* const, Args&&... args) {
        return function_ptr(std::forward<Args>(args)...);
    }

    template<class C, Result (C::*method_ptr)(Args...)>
    static Result method_stub(void* const object_ptr, Args&&... args) {
        return (static_cast<C*>(object_ptr)->*method_ptr)(std::forward<Args>(args)...);
    }

    template<class C, Result (C::*method_ptr)(Args...) const>
    static Result const_method_stub(void* const object_ptr, Args&&... args) {
        return (static_cast<C const*>(object_ptr)->*method_ptr)(std::forward<Args>(args)...);
    }

    template<class T>
    static std::enable_if_t<!(is_member_pair<T>::value || is_const_member_pair<T>::value), Result>
    functor_stub(void* const object_ptr, Args&&... args) {
        return (*static_cast<T*>(object_ptr))(std::forward<Args>(args)...);
    }

    template<class T>
    static std::enable_if_t<is_member_pair<T>::value || is_const_member_pair<T>::value, Result>
    functor_stub(void* const object_ptr, Args&&... args) {
        return (static_cast<T*>(object_ptr)->first->*static_cast<T*>(object_ptr)->second)(std::forward<Args>(args)...);
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    void* m_obj_ptr;
    stub_ptr_type m_stub_ptr{};
    deleter_type m_deleter;
    std::shared_ptr<void> m_storage;
    std::size_t m_storage_size;
};

NOTF_CLOSE_NAMESPACE

// std::hash ======================================================================================================== //

namespace std {

/// std::hash specialization for notf::Delegate.
template<class Result, class... Args>
struct hash<::notf::Delegate<Result(Args...)>> {
    size_t operator()(::notf::Delegate<Result(Args...)> const& delegate) const noexcept {
        NOTF_USING_NAMESPACE;
        return hash(pointer_hash(delegate.object_ptr_), pointer_hash(delegate.stub_ptr_));
    }
};

} // namespace std
