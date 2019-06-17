#include <iostream>
#include <cstdlib>

#include <boost/shared_ptr.hpp>

#include "notf/common/dynarray.hpp"



class SharedMagic {

public:
    using Ptr = boost::shared_ptr<SharedMagic>;

private:
    SharedMagic() = default;
    friend Ptr boost::make_shared<SharedMagic>();
    
public:
    static Ptr create() {
        auto result = boost::make_shared<SharedMagic>();
        result->m_self = result;
        return result;
    }

    ~SharedMagic(){
        std::cout << "The magic is gone!" << std::endl;
    }

    void clear() {
        m_self.reset();
    }

private:
    Ptr m_self;
};

void clear_the_magic(SharedMagic::Ptr& magic) {
    std::cout << "About to clear the magic" << std::endl;
    magic->clear();
    if(magic.unique()){
        std::cout << "Magic is unique" << std::endl;
    }
    magic->clear();
    magic->clear();
    magic->clear();
    std::cout << "Done clearing the magic" << std::endl;
}

void restore_the_magic(const u_char* buffer){

}

int main()
{
    {
        auto magic = SharedMagic::Ptr();
        const char* itr = std::launder(reinterpret_cast<const char*>(&magic));
        for(std::size_t i = 0; i < sizeof(SharedMagic::Ptr); ++i){
            std::cout << int(itr[0]) << std::endl;
        }
    }

    // std::cout << sizeof(SharedMagic::Ptr) << std::endl;
    // notf::DynArray<u_char> buffer(sizeof(SharedMagic::Ptr));
    // {
    //     std::cout << "Creating the magic" << std::endl;
    //     auto magic = SharedMagic::create();
    //     std::cout << "Saving the magic" << std::endl;
    //     std::memcpy(&buffer[0], std::launder(reinterpret_cast<const u_char*>(&magic)), sizeof(magic));
    //     std::cout << "Hiding the magic" << std::endl;
        
    //     SharedMagic::Ptr empty;
    //     std::memset(std::launder(reinterpret_cast<u_char*>(&magic)), 0, sizeof(empty));
    // }
    // {
    //     auto magic = SharedMagic::Ptr();
    //     std::cout << "Restoring the magic" << std::endl;
    //     std::memcpy(std::launder(reinterpret_cast<u_char*>(&magic)), &buffer[0], sizeof(magic));
    //     clear_the_magic(magic);
    //     std::cout << "Closing the bracket" << std::endl;
    // }
    // std::cout << "... or is it?" << std::endl;
}

// #include <iostream>
// #include <cstdlib>

// int main()
// {
//     std::cout << "Hello, notf!" << std::endl;
// }
