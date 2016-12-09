#ifdef DOCSTR
#error Another project is already defining the NoTF DOCSTR macro, you might want to rename it in order to compile.
#endif

/**
 * If you want to shave off all non-essential bits and pieces off the build,
 * defining NO_DOC at build time will get rid of all documentation strings of the Python bindings.
 * I was hoping for more savings though, at the moment the difference between uncompressed files is just over 100kb.
 */

#ifdef NO_DOC
#define DOCSTR(x) ""
#else
#define DOCSTR(x) x
#endif
