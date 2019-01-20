#include <exception>
#include <iostream>
#include <map>

struct Context {
private:
    class State {

    private:
        struct _BlendMode {
            _BlendMode() { std::cout << "Created BlendMode with value " << value << std::endl; }
            void operator=(const int new_value) {
                if (new_value != value) {
                    value = new_value;
                    std::cout << "Changed BlendMode to " << value << std::endl;
                }
            }
            operator int() const { return value; }

        private:
            int value = 0;
        };

        struct _TextureSlot {
            _TextureSlot(const int index) : index(index) {
                std::cout << "Created Texture slot " << index << " with value " << value << std::endl;
            }
            void operator=(const int new_value) {
                if (new_value != value) {
                    value = new_value;
                    std::cout << "Changed TextureSlot " << index << " to " << value << std::endl;
                }
            }
            operator int() const { return value; }

        private:
            const int index;
            int value = 0;
        };

        struct _TextureSlots {

            _TextureSlot& operator[](const int index) {
                if (index >= 10) { throw std::out_of_range("Texture slot is out of range (>= 10)"); }
                auto itr = slots.find(index);
                if (itr == slots.end()) { std::tie(itr, std::ignore) = slots.emplace(std::make_pair(index, index)); }
                return itr->second;
            }

        private:
            std::map<int, _TextureSlot> slots;
        };

    public:
        _BlendMode blend_mode;
        _TextureSlots texture_slots;

    };

    State state;

public:
    State* operator->() { return &state; }
};

int main() {
    Context context;
    context->blend_mode = 2;
    context->blend_mode = 5;
    context->blend_mode = 5;
    std::cout << "Current BlendMode is " << context->blend_mode;

    context->texture_slots[2] = 3;
    context->texture_slots[2] = 8;
    context->texture_slots[8] = 0;
    try {
        context->texture_slots[11] = 0;
    } catch (const std::out_of_range&) {
        std::cout << "Caught out of range error" << std::endl;
    }


}
