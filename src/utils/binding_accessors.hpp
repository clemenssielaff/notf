#ifdef NOTF_BINDINGS
#define PROTECTED_EXCEPT_FOR_BINDINGS public:
#define PRIVATE_EXCEPT_FOR_BINDINGS public:
#define CONST_EXCEPT_FOR_BINDINGS
#else
#define PROTECTED_EXCEPT_FOR_BINDINGS protected:
#define PRIVATE_EXCEPT_FOR_BINDINGS private:
#define CONST_EXCEPT_FOR_BINDINGS const
#endif