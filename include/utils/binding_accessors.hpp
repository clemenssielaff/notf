#ifdef NOTF_BINDINGS
#define protected_except_for_bindings public
#define private_except_for_bindings public
#define const_except_for_bindings
#else
#define protected_except_for_bindings protected
#define private_except_for_bindings private
#define const_except_for_bindings const
#endif
