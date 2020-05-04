#pragma once

// brekable scope =================================================================================================== //

/// Use `breakable_scope` to write:
///
///     breakable_scope {
///         ...
///         if(!condition){
///             break;
///         }
///         ...
///     }
///     else{ // called when the breakable_scope is exited via `break`
///         handle_error();
///     }
///
/// If you exit the scope via `continue`, it will break the scope without
/// executing the `else` section at the end.
///
#define NOTF_BREAKABLE_SCOPE                                                                                \
    for (auto _breakable_scope_variable = 0; _breakable_scope_variable < 2; _breakable_scope_variable += 1) \
        for (; _breakable_scope_variable < 2; _breakable_scope_variable += 2)                               \
            if (_breakable_scope_variable == 0)
