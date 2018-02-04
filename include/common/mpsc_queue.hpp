#pragma once
///
/// Code for the MpscQueue was adapted from:
///     https://int08h.com/post/ode-to-a-vyukov-queue/
/// and:
///     https://github.com/mstump/queues/blob/master/include/mpsc-queue.hpp
/// after a design from Dmitry Vyukov:
///     http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
///
/// @remark If you ever want to spend a fun afternoon premature optimizing stuff that probably doesn't need
/// optimization, you could investigate the use of a custom queue allocator here, turning the queue into a bounded queue
/// without any dynamic memory allocation, for the use-case where the queue is emptied out regularly by the consumer. In
/// that case, you could allocate a block of memory enough to hold `n` instances of aligned type T, and push new
/// instances onto the head. Whenever you flush the stack, you simply move the pointer to head memory back to the start
/// of the block.
/// ... That might screw with the non-blocking requirement though... so maybe not?

#include <atomic>

#include "common/meta.hpp"

namespace notf {

//====================================================================================================================//

/// C++ implementation of the non-intrusive Vyukov MPSC queue (FIFO).
/// The queue grows by pushing onto the `head` and poping from the `tail`.
template<typename T>
class MpscQueue final {

    /// Data Type.
    using Data = T;

    /// Node in the queue's linked list.
    struct Node {
        /// Next node.
        std::atomic<Node*> next;

        /// Data
        Data data;
    };

    // methods -------------------------------------------------------------------------------------------------------//
public:
    DISALLOW_COPY_AND_ASSIGN(MpscQueue)

    /// Constructor.
    MpscQueue() : m_head(new Node), m_tail(m_head.load(std::memory_order_relaxed))
    {
        Node* head = m_head.load(std::memory_order_relaxed);
        head->next.store(nullptr, std::memory_order_relaxed);
    }

    /// Destructor.
    ~MpscQueue() noexcept
    {
        Data output;
        while (pop(output)) {
        }
        Node* head = m_head.load(std::memory_order_relaxed);
        delete head;
    }

    /// Push another item onto the queue.
    /// @param item     New item.
    void push(Data&& item)
    {
        Node* node = new Node;
        node->data = std::move(item);
        node->next.store(nullptr, std::memory_order_relaxed);

        Node* prev_head = m_head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }

    /// Returns the oldest item in the queue, if there is one.
    /// @param item     [out] Is overwritten with the oldest item in the queue, if it exists.
    /// @returns        True if `item` now contains the requested item, false if the queue is empty.
    bool pop(Data& item) noexcept
    {
        Node* tail = m_tail.load(std::memory_order_relaxed);
        Node* next = tail->next.load(std::memory_order_acquire);
        if (!next) {
            return false;
        }

        item = std::move(next->data);
        m_tail.store(next, std::memory_order_release);
        delete tail;
        return true;
    }

    // fields --------------------------------------------------------------------------------------------------------//
private:
    /// Stub item at the front, is moved when a new item is pushed onto the queue.
    std::atomic<Node*> m_head;

    /// Dataless item at the back.
    std::atomic<Node*> m_tail;
};

} // namespace notf
