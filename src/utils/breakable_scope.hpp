#pragma once

/// Use `notf_breakable_scope` to write:
///
///     notf_breakable_scope {
///         ...
///         if(!condition){
///             break;
///         }
///         ...
///     }
///     else{ // called when the notf_breakable_scope is exited via `break`
///         handle_error();
///     }
///
/// If you exit the scope via `continue`, it will break the scope without
/// executing the `else` section at the end.
///
#define notf_breakable_scope                                                                                  \
    for (auto __notf_breakable_scope_condition_variable = 0; __notf_breakable_scope_condition_variable < 2;   \
         __notf_breakable_scope_condition_variable += 1)                                                      \
        for (; __notf_breakable_scope_condition_variable < 2; __notf_breakable_scope_condition_variable += 2) \
            if (__notf_breakable_scope_condition_variable == 0)
