#pragma once

#include "./publisher.hpp"

NOTF_OPEN_NAMESPACE

// relay ============================================================================================================ //

/// Base Relay template, combining a Subscriber and Producer (potentially of different types) into a single object.
template<class I, class O = I>
struct Relay : public Subscriber<I>, public Publisher<O> {

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const std::exception& exception) override { this->error(exception); }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete() override { this->complete(); }
};

// ================================================================================================================== //

/// Default specialization for Relays that have the same input and output types.
template<class T>
struct Relay<T, T> : public Subscriber<T>, public Publisher<T> {

    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const T& value) override { this->next(value); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const std::exception& exception) override { this->error(exception); }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete() override { this->complete(); }
};

// ================================================================================================================== //

/// Specialization for Relays that connect a data-producing upstream to a "NoData" downstream.
template<class T>
struct Relay<T, NoData> : public Subscriber<T>, public Publisher<NoData> {

    /// Subscriber "next" operation, forwards to the Producer's "publish" operation by default.
    void on_next(const T& /* ignored */) override { this->next(); }

    /// Subscriber "error" operation, forwards to the Producer's "fail" operation by default.
    void on_error(const std::exception& exception) override { this->error(exception); }

    /// Subscriber "complete" operation, forwards to the Producer's "complete" operation by default.
    void on_complete() override { this->complete(); }
};

NOTF_CLOSE_NAMESPACE
