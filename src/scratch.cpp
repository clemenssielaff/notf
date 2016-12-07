#include <memory>
#include <assert.h>

class Brap : public std::enable_shared_from_this<Brap> {
public:
    Brap() = default;
};


//int main() {
int notmain() {

    std::shared_ptr<Brap> brap = std::make_shared<Brap>();

    int i = brap.use_count();
    return 0;
}
