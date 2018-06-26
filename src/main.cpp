#include <iostream>

//#include "framebuffer_example.hpp"
//#include "line_example.hpp"
//#include "rendermanager_example.hpp"
//#include "renderthread_example.hpp"
//#include "shape_example.hpp"
//#include "text_example.hpp"
//#include "wireframe_example.hpp"
#include "smoke_example.hpp"

#include "common/meta.hpp"
NOTF_USING_NAMESPACE;

int main(int argc, char* argv[])
{
    // disables the synchronization standard streams
    // (for detail see: https://stackoverflow.com/a/31165481)
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    //    return shape_main(argc, argv);
    //    return line_main(argc, argv);
    //    return wireframe_main(argc, argv);
    //    return framebuffer_main(argc, argv);
    //    return text_main(argc, argv);
    //    return property_main(argc, argv);
    //    return rendermanager_main(argc, argv);
    //    return renderthread_main(argc, argv);
    return smoke_main(argc, argv);
    //    (void)(argc);
    //    (void)(argv);
    //    return 0;
}
