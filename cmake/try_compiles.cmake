# In order to pass the compile and linker flags required to use libc++, we need to get them into the `try_compile`
# command somehow. I didn't find a way to do it using the CMAKE_FLAGS option, but setting policies "CMP0056" and
# "CMP0066" (and I suppose "CMP0067"), at least the `CMAKE_CXX_FLAGS` and `CMAKE_EXE_LINKER_FLAGS` are passed correctly.
# However, they are not actually defined at this point, so we have to read them manually from the directory property,
# transform the string and feed them back into the magic variables to the following `try_compile` command works.
get_directory_property(NOTF_COMPILE_OPTIONS "COMPILE_OPTIONS")
string (REPLACE ";" " " NOTF_COMPILE_OPTIONS "${NOTF_COMPILE_OPTIONS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${NOTF_COMPILE_OPTIONS}")

get_directory_property(NOTF_LINK_OPTIONS "LINK_OPTIONS")
string (REPLACE ";" " " NOTF_LINK_OPTIONS "${NOTF_LINK_OPTIONS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${NOTF_LINK_OPTIONS}")

# DECLVAL ##############################################################################################################

# GCC seems to have a bug that causes the expression
#     decltype(delcval<T>())
# to be evaluated at compile time, triggering a static assert in its definition of declval.
# If the test compilation fails, we use our own definition supplied by cppreference
# (https://en.cppreference.com/w/cpp/utility/declval) which does the same thing but doesn't assert.
try_compile(
    NOTF_COMPILER_HAS_DECLVAL
    "${CMAKE_BINARY_DIR}"
    "${NOTF_ROOT_DIR}/cmake/try_compiles/declval.cpp"
    )
if(NOTF_COMPILER_HAS_DECLVAL)
    message(STATUS "${PROJECT_NAME}: Compiler supports `decltype(std::declval<T>())` ✔")
else()
    message(STATUS "${PROJECT_NAME}: Compiler DOES NOT support `decltype(std::declval<T>())` ✘")
endif()
