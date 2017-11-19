//#include "examples/framebuffer_example.hpp"
//#include "examples/wireframe_example.hpp"

// int main(int argc, char* argv [])
//{
//    return wireframe_main(argc, argv);
//    return framebuffer_main(argc, argv);
//}

#include <iostream>
#include <regex>

#include "common/system.hpp"
#include "utils/narrow_cast.hpp"

using namespace notf;

int main(int, char* [])
{
    std::string original = load_file("/home/clemens/code/notf/res/shaders/line.frag");
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
    std::cout << original << std::endl;
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;

    size_t injection_index = std::string::npos;
    {
        std::regex extensions_regex(R"==(\n\s*#extension\s*\w*\s*:\s*(?:require|enable|warn|disable)\s*\n)==");

        std::smatch extensions;
        std::regex_search(original, extensions, extensions_regex);
        if (!extensions.empty()) {
            // if the shader contains one or more #extension strings, we have to inject the #defines after those
            injection_index = 0;
            std::string remainder;
            do {
                remainder = extensions.suffix();
                injection_index += narrow_cast<size_t>(extensions.position(extensions.size() - 1)
                                                       + extensions.length(extensions.size() - 1));
                std::regex_search(remainder, extensions, extensions_regex);
            } while (!extensions.empty());
        }
        else {
            // otherwise, look for the mandatory #version string
            std::smatch version;
            std::regex_search(original, version, std::regex(R"==(\n\s*#version\s*\d{3}\s*es[ \t]*\n)=="));
            if (!version.empty()) {
                injection_index
                    = narrow_cast<size_t>(version.position(version.size() - 1) + version.length(version.size() - 1));
            }
        }
    }
    if (injection_index == std::string::npos) {
        std::cout << "NO FIND" << std::endl;
    }
    else {
        std::cout << "Injection index: " << injection_index << std::endl;
    }

    std::string prefix    = original.substr(0, injection_index);
    std::string injection = "\n#define NOTF_IS_AWESOME true\n";
    std::string postfix   = original.substr(injection_index, std::string::npos);

    std::string modified = prefix + injection + postfix;
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
    std::cout << modified << std::endl;
    std::cout << "---------------------------------------------------------------------------------------" << std::endl;
}
