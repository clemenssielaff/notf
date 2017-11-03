#pragma once

#include "common/meta.hpp"

/// Instead of writing
///
///     if(a){
///         ...
///         if(b) {
///             ...
///             if(c){
///                 ...
///             }
///             else {
///                 handle_error();
///             }
///         }
///         else {
///             handle_error();
///         }
///     }
///     else {
///         handle_error();
///     }
///
/// you can use `breakable_scope` to write:
///
///     breakable_scope {
///         ...
///         if(!a){
///             break;
///         }
///         ...
///         if(!b){
///             break;
///         }
///         ...
///         if(!c){
///             break;
///         }
///         ...
///     }
///     else{ // called when the breakable_scope is exited via `break`
///         handle_error();
///     }
///
/// If you exit the scope via `continue`, it will break the scope without executing the `else` section at the end.
/// 
/// A simpler form of the macro is:
/// 
///     #define breakable_scope for (auto tkn = 0; tkn < 2; tkn += 1) for (; tkn < 2; tkn += 2) if (YYY_BS_TOKEN < 1)
/// 
#define breakable_scope                                                     \
    for (auto MACRO_CONCAT(__breakable_scope_token_prefix_, __LINE__) = 0;  \
         MACRO_CONCAT(__breakable_scope_token_prefix_, __LINE__) < 2;       \
         MACRO_CONCAT(__breakable_scope_token_prefix_, __LINE__) += 1)      \
        for (; MACRO_CONCAT(__breakable_scope_token_prefix_, __LINE__) < 2; \
             MACRO_CONCAT(__breakable_scope_token_prefix_, __LINE__) += 2)  \
            if (MACRO_CONCAT(__breakable_scope_token_prefix_, __LINE__) < 1)


