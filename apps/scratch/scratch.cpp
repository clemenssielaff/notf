#include <iostream>

#include "notf/app/application.hpp"

NOTF_USING_NAMESPACE;

int main()
{
    TheApplication::get();

    NOTF_LOG_INFO("juhuu!");

    return 0;
}
