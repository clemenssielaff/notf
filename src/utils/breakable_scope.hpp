#pragma once

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
#define breakable_scope                                                                             \
    for (auto __breakable_scope_condition_variable = 0; __breakable_scope_condition_variable < 2;   \
         __breakable_scope_condition_variable += 1)                                                 \
        for (; __breakable_scope_condition_variable < 2; __breakable_scope_condition_variable += 2) \
            if (__breakable_scope_condition_variable == 0)
