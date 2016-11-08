#include "test/catch.hpp"

#include <memory>
#include <vector>

#include "common/random.hpp"
using notf::get_random_engine;

#include "core/zhierarchy.hpp"
using notf::ZIterator;
using notf::ZNode;

SCENARIO("ZNodes form a hierarchy that can be modified", "[core][zhierarchy]")
{
    auto& random_engine = get_random_engine();

    WHEN("ZNodes are simply stacked in the order they are created")
    {
        const size_t size = static_cast<size_t>(random_engine.uniform(1, 256));

        std::vector<std::unique_ptr<ZNode>> owner;
        owner.reserve(size);

        owner.emplace_back(std::make_unique<ZNode>(nullptr));
        ZNode* root = owner.front().get();

        for (size_t i = 1; i < size; ++i) {
            owner.emplace_back(std::make_unique<ZNode>(nullptr));
            ZNode* node = owner.at(i).get();
            node->place_on_top_of(root);
        }

        THEN("flattening the hierarchy will result in the creation order")
        {
            std::vector<ZNode*> flattened = root->flatten();
            REQUIRE(flattened.size() == size);
            for (size_t i = 0; i < size; ++i) {
                REQUIRE(flattened[i] == owner[i].get());
            }
        }
    }

    WHEN("ZNodes are always inserted in the back")
    {
        const size_t size = static_cast<size_t>(random_engine.uniform(1, 256));

        std::vector<std::unique_ptr<ZNode>> owner;
        owner.reserve(size);

        owner.emplace_back(std::make_unique<ZNode>(nullptr));
        ZNode* root = owner.front().get();

        for (size_t i = 1; i < size; ++i) {
            owner.emplace_back(std::make_unique<ZNode>(nullptr));
            ZNode* node = owner.at(i).get();
            node->place_on_bottom_of(root);
        }

        THEN("flattening the hierarchy will result in the reverse creation order")
        {
            std::vector<ZNode*> flattened = root->flatten();
            REQUIRE(flattened.size() == size);
            for (size_t i = 0; i < size; ++i) {
                REQUIRE(flattened[i] == owner[size-i-1].get());
            }
        }
    }
}
