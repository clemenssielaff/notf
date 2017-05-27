#pragma once

#include <memory>
#include <set>

#include "graphics/cell/command_buffer.hpp"

namespace notf {

/**********************************************************************************************************************/

/**
 *
 * The Vault
 * =========
 * Commands have to be a fixed size at compile time, yet not all arguments to paint calls are of fixed size.
 * Images for example, or strings that are rendered as text.
 * In order to store them savely inside a Command, we pack them into a shared_ptr<> and serialize the shared_ptr into
 * the command buffer.
 * At the same time, to make sure that the actual object is not destroyed, we put another (live) shared_ptr into the
 * Cell's vault, where it looses its type.
 * It is called a vault because it keeps stuff save inside and also, because once it's in there - you are not getting
 * it out again.
 * The Painter, when drawing into a Cell, clears both the command buffer and the vault, freeing all previously alocated
 * memory that has gone out of scope.
 */
class Cell {

    friend class Painter;

public: // methods
    DEFINE_SHARED_POINTER_TYPES(Cell)

    Cell() = default;

    /** The Painter Command buffer of this Cell. */
    const PainterCommandBuffer& get_commands() const { return m_commands; }

    /** Clears all contents of the Cell. */
    void clear()
    {
        m_commands.clear();
        m_vault.clear();
    }

private: // fields
    /** Painterpreter commands to paint this Cell. */
    PainterCommandBuffer m_commands;

    /** Shared pointers coresponding to serialized shared_ptrs in the command buffer. */
    std::set<std::shared_ptr<void>> m_vault;
};

} // namespace notf
