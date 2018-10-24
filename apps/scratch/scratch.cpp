#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "notf/common/mutex.hpp"
#include "notf/meta/config.hpp"
#include "notf/meta/types.hpp"

NOTF_USING_NAMESPACE;

class Appblub {

    Appblub() = default;

    static Appblub& _get()
    {
        static Appblub instance;
        std::cout << "Fear not, for it is blubbed now" << std::endl;
        instance.m_is_blubbed = true;
        return instance;
    }

public:
    static Appblub& get()
    {
        static Appblub& instance = _get();
        return instance;
    }

    void do_the_blub()
    {
        if (m_is_blubbed) { std::cout << "The Blub!" << std::endl; }
        else {
            std::cout << "alas, it was not the blub" << std::endl;
        }
    }

private:
    bool m_is_blubbed = false;
    Mutex m_mutex;
};

int main()
{
    Appblub::get().do_the_blub();
    Appblub::get().do_the_blub();
    Appblub::get().do_the_blub();
    Appblub::get().do_the_blub();
    return 0;
}
