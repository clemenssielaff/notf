#include <iostream>

#include "framebuffer_example.hpp"
#include "line_example.hpp"
#include "mpsc_example.hpp"
#include "shape_example.hpp"
#include "text_example.hpp"
#include "wireframe_example.hpp"

int main(int argc, char* argv[])
{
    //    return shape_main(argc, argv);
    //    return line_main(argc, argv);
    //    return wireframe_main(argc, argv);
    //    return framebuffer_main(argc, argv);
    //    return text_main(argc, argv);
    return mpsc_main(argc, argv);
    //    (void)(argc);
    //    (void)(argv);
    //    return 0;
}
