#pragma once

#define ENABLE_IF_REAL(TYPE) typename = std::enable_if_t<std::is_floating_point<typename std::decay<TYPE>::type>::value>
#define DISABLE_IF_REAL(TYPE) typename = std::enable_if_t<!std::is_floating_point<typename std::decay<TYPE>::type>::value>
#define ENABLE_IF_INT(TYPE) typename = std::enable_if_t<std::is_integral<typename std::decay<TYPE>::type>::value>
#define DISABLE_IF_INT(TYPE) typename = std::enable_if_t<!std::is_integral<typename std::decay<TYPE>::type>::value>

