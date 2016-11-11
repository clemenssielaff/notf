#include "test/catch.hpp"

#include <assert.h>
#include <memory>
#include <set>
#include <vector>

#ifdef _TEST
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define protected public
#define private public
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

#include "common/const.hpp"
#include "common/log.hpp"
#include "common/random.hpp"
#include "core/layout_root.hpp"
#include "core/object_manager.hpp"
#include "core/znode.hpp"
#include "dynamic/layout/stack_layout.hpp"
using namespace notf;

namespace { // anonymous

/** Produces a simple hierarchy of 5 ZNodes.
 *
 *                  root
 *                 /    \
 *          mid_left     mid_right
 *         /                      \
 *     left                        right
 */
std::vector<std::unique_ptr<ZNode>> produce_five_hierarchy()
{
    std::vector<std::unique_ptr<ZNode>> result;
    for (auto i = 0; i < 5; ++i) {
        result.emplace_back(std::make_unique<ZNode>(nullptr));
    }

    ZNode* left = result.at(0).get();
    ZNode* mid_left = result.at(1).get();
    ZNode* root = result.at(2).get();
    ZNode* mid_right = result.at(3).get();
    ZNode* right = result.at(4).get();

    left->place_on_bottom_of(mid_left);
    mid_left->place_on_bottom_of(root);
    mid_right->place_on_top_of(root);
    right->place_on_top_of(mid_right);

    return result;
}

/*
 *                               A
 *                              /
 *               +---+---+-----+
 *              /    |    \     \
 *             B     C     D     E
 *            /     /       \     \
 *           F     G         H     +---+---+
 *          / \                     \   \   \
 *         I   J                     K   L   M
 */
std::vector<std::shared_ptr<StackLayout>> produce_example_a()
{
    std::shared_ptr<StackLayout> a = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> b = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> c = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> d = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> e = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> f = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> g = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> h = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> i = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> j = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> k = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> l = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> m = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);

    b->place_on_bottom_of(a);
    c->place_above(b);
    d->place_above(c);
    e->place_above(d);
    f->place_on_bottom_of(b);
    g->place_on_bottom_of(c);
    h->place_on_top_of(d);
    i->place_on_bottom_of(f);
    j->place_on_top_of(f);
    k->place_on_top_of(e);
    l->place_above(k);
    m->place_on_top_of(e);

    return {a, b, c, d, e, f, g, h, i, j, k, l, m};
}

/*
 *             A
 *              \
 *               +---+---+-----+
 *              /    |    \     \
 *             B     C     D     E
 *            /     /       \     \
 *           F     G         H     +---+---+
 *          / \                     \   \   \
 *         I   J                     K   L   M
 */
std::vector<std::shared_ptr<StackLayout>> produce_example_b()
{
    std::shared_ptr<StackLayout> a = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> b = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> c = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> d = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> e = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> f = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> g = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> h = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> i = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> j = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> k = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> l = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
    std::shared_ptr<StackLayout> m = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);

    b->place_above(a);
    c->place_above(b);
    d->place_above(c);
    e->place_above(d);
    f->place_on_bottom_of(b);
    g->place_on_bottom_of(c);
    h->place_on_top_of(d);
    i->place_on_bottom_of(f);
    j->place_on_top_of(f);
    k->place_on_top_of(e);
    l->place_above(k);
    m->place_on_top_of(e);

    return {a, b, c, d, e, f, g, h, i, j, k, l, m};
}

} // namespace anonymous

SCENARIO("ZNodes form a hierarchy that can be modified", "[core][zhierarchy]")
{
    auto& random_engine = get_random_engine();
    //    random_engine.seed();

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
            std::vector<ZNode*> flattened = root->flatten_hierarchy();
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
            std::vector<ZNode*> flattened = root->flatten_hierarchy();
            REQUIRE(flattened.size() == size);
            for (size_t i = 0; i < size; ++i) {
                REQUIRE(flattened[i] == owner[size - i - 1].get());
            }
        }
    }

    WHEN("a ZNode tries to move above or below another ZNode that has no parent")
    {

        std::unique_ptr<ZNode> left_owner1 = std::make_unique<ZNode>(nullptr);
        std::unique_ptr<ZNode> right_owner1 = std::make_unique<ZNode>(nullptr);
        ZNode* left1 = left_owner1.get();
        ZNode* right1 = right_owner1.get();
        right1->place_above(left1);

        std::unique_ptr<ZNode> left_owner2 = std::make_unique<ZNode>(nullptr);
        std::unique_ptr<ZNode> right_owner2 = std::make_unique<ZNode>(nullptr);
        ZNode* left2 = left_owner2.get();
        ZNode* right2 = right_owner2.get();
        left2->place_below(right2);

        THEN("it is instead parented to the other node")
        {
            REQUIRE(left1->m_left_children.empty());
            REQUIRE(left1->m_right_children.size() == 1);
            REQUIRE(left1->m_right_children.at(0) == right1);
            REQUIRE(left1->m_num_left_descendants == 0);
            REQUIRE(left1->m_num_right_descendants == 1);
            REQUIRE(right1->m_parent == left1);

            REQUIRE(right2->m_right_children.empty());
            REQUIRE(right2->m_left_children.size() == 1);
            REQUIRE(right2->m_left_children.at(0) == left2);
            REQUIRE(right2->m_num_left_descendants == 1);
            REQUIRE(right2->m_num_right_descendants == 0);
            REQUIRE(left2->m_parent == right2);
        }
    }

    WHEN("a simple 5-hierarchy is created")
    {
        std::vector<std::unique_ptr<ZNode>> hierarchy = produce_five_hierarchy();

        THEN("it actually works")
        {
            REQUIRE(hierarchy.size() == 5);

            ZNode* left = hierarchy.at(0).get();
            ZNode* mid_left = hierarchy.at(1).get();
            ZNode* root = hierarchy.at(2).get();
            ZNode* mid_right = hierarchy.at(3).get();
            ZNode* right = hierarchy.at(4).get();

            REQUIRE(left->m_parent == mid_left);
            REQUIRE(left->m_left_children.empty());
            REQUIRE(left->m_right_children.empty());
            REQUIRE(left->m_num_left_descendants == 0);
            REQUIRE(left->m_num_right_descendants == 0);
            REQUIRE(left->m_placement == ZNode::left);
            REQUIRE(left->m_index == 0);

            REQUIRE(mid_left->m_parent == root);
            REQUIRE(mid_left->m_left_children.size() == 1);
            REQUIRE(mid_left->m_left_children.at(0) == left);
            REQUIRE(mid_left->m_right_children.empty());
            REQUIRE(mid_left->m_num_left_descendants == 1);
            REQUIRE(mid_left->m_num_right_descendants == 0);
            REQUIRE(mid_left->m_placement == ZNode::left);
            REQUIRE(mid_left->m_index == 0);

            REQUIRE(root->m_parent == nullptr);
            REQUIRE(root->m_left_children.size() == 1);
            REQUIRE(root->m_left_children.at(0) == mid_left);
            REQUIRE(root->m_right_children.size() == 1);
            REQUIRE(root->m_right_children.at(0) == mid_right);
            REQUIRE(root->m_num_left_descendants == 2);
            REQUIRE(root->m_num_right_descendants == 2);
            REQUIRE(root->m_placement == ZNode::left);
            REQUIRE(root->m_index == 0);

            REQUIRE(mid_right->m_parent == root);
            REQUIRE(mid_right->m_left_children.empty());
            REQUIRE(mid_right->m_right_children.size() == 1);
            REQUIRE(mid_right->m_right_children.at(0) == right);
            REQUIRE(mid_right->m_num_left_descendants == 0);
            REQUIRE(mid_right->m_num_right_descendants == 1);
            REQUIRE(mid_right->m_placement == ZNode::right);
            REQUIRE(mid_right->m_index == 0);

            REQUIRE(right->m_parent == mid_right);
            REQUIRE(right->m_left_children.empty());
            REQUIRE(right->m_right_children.empty());
            REQUIRE(right->m_num_left_descendants == 0);
            REQUIRE(right->m_num_right_descendants == 0);
            REQUIRE(right->m_placement == ZNode::right);
            REQUIRE(right->m_index == 0);
        }
    }

    WHEN("ZNodes are pre- and appended alternatingly at the end")
    {
        const size_t size = static_cast<size_t>(random_engine.uniform(1, 256));

        std::vector<std::unique_ptr<ZNode>> owner;
        owner.reserve(size);

        owner.emplace_back(std::make_unique<ZNode>(nullptr));
        ZNode* root = owner.front().get();

        bool append = true;
        for (size_t i = 1; i < size; ++i) {
            if (append) {
                owner.emplace_back(std::make_unique<ZNode>(nullptr));
                ZNode* node = owner.back().get();
                node->place_on_top_of(root);
            }
            else {
                owner.emplace(owner.begin(), std::make_unique<ZNode>(nullptr));
                ZNode* node = owner.front().get();
                node->place_on_bottom_of(root);
            }
            append = !append;
        }

        THEN("flattening the hierarchy will result in the correct order")
        {
            std::vector<ZNode*> flattened = root->flatten_hierarchy();
            REQUIRE(flattened.size() == size);
            for (size_t i = 0; i < size; ++i) {
                REQUIRE(flattened[i] == owner[i].get());
            }
        }
    }

    WHEN("a ZNode in a hierarchy is moved into the same position again")
    {
        std::vector<std::unique_ptr<ZNode>> hierarchy = produce_five_hierarchy();
        ZNode* left = hierarchy.at(0).get();
        ZNode* mid_left = hierarchy.at(1).get();
        ZNode* root = hierarchy.at(2).get();
        ZNode* mid_right = hierarchy.at(3).get();
        ZNode* right = hierarchy.at(4).get();

        left->place_below(mid_left);
        left->place_on_bottom_of(mid_left);
        mid_left->place_on_bottom_of(root);
        mid_left->place_below(root);

        mid_right->place_above(root);
        mid_right->place_on_top_of(root);
        right->place_on_top_of(mid_right);
        right->place_above(mid_right);

        THEN("flattening the hierarchy will result in the correct order")
        {
            std::vector<ZNode*> flattened = root->flatten_hierarchy();
            for (size_t i = 0; i < 5; ++i) {
                REQUIRE(flattened[i] == hierarchy[i].get());
            }
        }
    }

    WHEN("a ZNode is moved relative to itself")
    {
        auto root_owner = std::make_unique<ZNode>(nullptr);
        ZNode* root = root_owner.get();

        root->place_above(root);
        root->place_below(root);
        root->place_on_top_of(root);
        root->place_on_bottom_of(root);

        THEN("nothing happens")
        {
            REQUIRE(root->m_parent == nullptr);
            REQUIRE(root->m_left_children.empty());
            REQUIRE(root->m_right_children.empty());
            REQUIRE(root->m_num_left_descendants == 0);
            REQUIRE(root->m_num_right_descendants == 0);
            REQUIRE(root->m_placement == ZNode::left);
            REQUIRE(root->m_index == 0);
        }
    }

    WHEN("ZNodes are pre- and appended alternatingly in the middle")
    {
        const size_t size = static_cast<size_t>(random_engine.uniform(1, 256));

        std::vector<std::unique_ptr<ZNode>> owner;
        owner.reserve(size);

        owner.emplace_back(std::make_unique<ZNode>(nullptr));
        ZNode* root = owner.front().get();

        bool append = true;
        for (size_t i = 1; i < size; ++i) {
            std::unique_ptr<ZNode> node_owner = std::make_unique<ZNode>(nullptr);
            ZNode* node = node_owner.get();
            auto it = owner.begin();
            while (it->get() != root && it != owner.end()) {
                ++it;
            }
            assert(it != owner.end());
            if (append) {
                node->place_above(root);
                owner.emplace(++it, std::move(node_owner));
            }
            else {
                node->place_below(root);
                owner.emplace(it, std::move(node_owner));
            }
            append = !append;
        }

        THEN("flattening the hierarchy will result in the correct order")
        {
            std::vector<ZNode*> flattened = root->flatten_hierarchy();
            REQUIRE(flattened.size() == size);
            for (size_t i = 0; i < size; ++i) {
                REQUIRE(flattened[i] == owner[i].get());
            }
        }
    }

    WHEN("a ZNode is placed into the hierarchy below itself")
    {
        std::vector<std::unique_ptr<ZNode>> hierarchy = produce_five_hierarchy();
        ZNode* left = hierarchy.at(0).get();
        ZNode* mid_left = hierarchy.at(1).get();
        ZNode* root = hierarchy.at(2).get();
        ZNode* mid_right = hierarchy.at(3).get();
        ZNode* right = hierarchy.at(4).get();

        THEN("it will throw an error but keep in a consistent state")
        {
            REQUIRE_THROWS(root->place_above(mid_right));
            REQUIRE_THROWS(mid_left->place_below(left));
            REQUIRE_THROWS(mid_right->place_on_top_of(right));
            REQUIRE_THROWS(root->place_on_bottom_of(mid_left));

            std::vector<ZNode*> flattened = root->flatten_hierarchy();
            for (size_t i = 0; i < 5; ++i) {
                REQUIRE(flattened[i] == hierarchy[i].get());
            }
        }
    }

    WHEN("a ZHierarchy is randomly constructed")
    {
        const size_t size = static_cast<size_t>(random_engine.uniform(24, 1024));

        std::vector<std::unique_ptr<ZNode>> owner;
        owner.reserve(size);

        owner.emplace_back(std::make_unique<ZNode>(nullptr));
        ZNode* root = owner.front().get();

        for (size_t i = 0; i < size - 1; ++i) {
            const int pos = random_engine.uniform(0, static_cast<int>(i));
            const int op = random_engine.uniform(0, 3);

            std::unique_ptr<ZNode> node_owner = std::make_unique<ZNode>(nullptr);
            ZNode* node = node_owner.get();
            ZNode* other_node = owner.at(static_cast<size_t>(pos)).get();

            switch (op) {
            case 0:
                node->place_above(other_node);
                break;
            case 1:
                node->place_below(other_node);
                break;
            case 2:
                node->place_on_bottom_of(other_node);
                break;
            case 3:
                node->place_on_top_of(other_node);
                break;
            }
            owner.emplace_back(std::move(node_owner));
        }

        THEN("flattening the hierarchy will account for all ZNodes")
        {
            std::vector<ZNode*> flattened = root->flatten_hierarchy();
            std::set<ZNode*> flattened_set(flattened.begin(), flattened.end());
            REQUIRE(flattened_set.size() == size);

            REQUIRE(root->m_num_left_descendants + root->m_num_right_descendants + 1 == size);
        }
    }

    WHEN("constructing example a")
    {
        std::vector<std::shared_ptr<StackLayout>> owner = produce_example_a();
        assert(owner.size() == 13);

        auto a = owner[0]->m_znode.get();
        auto b = owner[1]->m_znode.get();
        auto c = owner[2]->m_znode.get();
        auto d = owner[3]->m_znode.get();
        auto e = owner[4]->m_znode.get();
        auto f = owner[5]->m_znode.get();
        auto g = owner[6]->m_znode.get();
        auto h = owner[7]->m_znode.get();
        auto i = owner[8]->m_znode.get();
        auto j = owner[9]->m_znode.get();
        auto k = owner[10]->m_znode.get();
        auto l = owner[11]->m_znode.get();
        auto m = owner[12]->m_znode.get();

        std::vector<ZNode*> expected = {i, f, j, b, g, c, d, h, e, k, l, m, a};

        THEN("flattening it will produce the correct result")
        {
            std::vector<ZNode*> flattened = a->flatten_hierarchy();
            REQUIRE(flattened.size() == expected.size());
            for (size_t i = 0; i < flattened.size(); ++i) {
                REQUIRE(flattened[i] == expected[i]);
            }
        }
    }

    WHEN("constructing example b")
    {
        std::vector<std::shared_ptr<StackLayout>> owner = produce_example_b();
        assert(owner.size() == 13);

        auto a = owner[0]->m_znode.get();
        auto b = owner[1]->m_znode.get();
        auto c = owner[2]->m_znode.get();
        auto d = owner[3]->m_znode.get();
        auto e = owner[4]->m_znode.get();
        auto f = owner[5]->m_znode.get();
        auto g = owner[6]->m_znode.get();
        auto h = owner[7]->m_znode.get();
        auto i = owner[8]->m_znode.get();
        auto j = owner[9]->m_znode.get();
        auto k = owner[10]->m_znode.get();
        auto l = owner[11]->m_znode.get();
        auto m = owner[12]->m_znode.get();

        std::vector<ZNode*> expected = {a, i, f, j, b, g, c, d, h, e, k, l, m};

        THEN("flattening it will produce the correct result")
        {
            std::vector<ZNode*> flattened = a->flatten_hierarchy();
            REQUIRE(flattened.size() == expected.size());
            for (size_t i = 0; i < flattened.size(); ++i) {
                REQUIRE(flattened[i] == expected[i]);
            }
        }
    }

    /*
     *            A
     *             \
     *              +--+--+
     *              |  |  |
     *              B  C  D
     *                  \
     *                   +--+
     *                   |  |
     *                   E  F
     */
    WHEN("LayoutItems are created and parented without regard to z values")
    {
        Handle root_handle = Application::get_instance().get_object_manager()._get_next_handle();
        std::shared_ptr<LayoutRoot> root = LayoutRoot::create(root_handle, {});
        std::shared_ptr<StackLayout> a = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> b = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> c = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> d = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> e = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> f = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);

        root->set_item(a);
        a->add_item(b);
        a->add_item(c);
        a->add_item(d);
        c->add_item(e);
        c->add_item(f);

        std::vector<ZNode*> expected = {
            a->m_znode.get(),
            b->m_znode.get(),
            c->m_znode.get(),
            e->m_znode.get(),
            f->m_znode.get(),
            d->m_znode.get(),
        };

        THEN("all children just stack on top of each other")
        {
            std::vector<ZNode*> flattened = a->m_znode->flatten_hierarchy();
            REQUIRE(flattened.size() == expected.size());
            for (size_t i = 0; i < flattened.size(); ++i) {
                REQUIRE(flattened[i] == expected[i]);
            }
        }
    }

    /*
     *    A                    A
     *     \                    \
     *      +--+--+              +--+
     *      |  |  |              |  |
     *      B  C  D      =>      B  D
     *          \                    \
     *           +--+                 C
     *           |  |                  \
     *           E  F                   +--+
     *                                  |  |
     *                                  E  F
     */
    WHEN("A LayoutItem is moved within the LayoutItem hierarchy, its ZNode is moved on top of the new parent")
    {
        Handle root_handle = Application::get_instance().get_object_manager()._get_next_handle();
        std::shared_ptr<LayoutRoot> root = LayoutRoot::create(root_handle, {});
        std::shared_ptr<StackLayout> a = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> b = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> c = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> d = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> e = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> f = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);

        root->set_item(a);
        a->add_item(b);
        a->add_item(c);
        a->add_item(d);
        c->add_item(e);
        c->add_item(f);

        d->add_item(c);

        std::vector<ZNode*> expected = {
            a->m_znode.get(),
            b->m_znode.get(),
            d->m_znode.get(),
            c->m_znode.get(),
            e->m_znode.get(),
            f->m_znode.get(),
        };

        THEN("all children just stack on top of each other")
        {
            std::vector<ZNode*> flattened = a->m_znode->flatten_hierarchy();
            REQUIRE(flattened.size() == expected.size());
            for (size_t i = 0; i < flattened.size(); ++i) {
                REQUIRE(flattened[i] == expected[i]);
            }
        }
    }

    WHEN("a ZNode is deleted because its LayoutItem is deleted")
    {
        std::shared_ptr<StackLayout> root = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        std::shared_ptr<StackLayout> left = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
        root->add_item(left);
        {
            std::shared_ptr<StackLayout> mid = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
            mid->place_below(root);
            left->place_on_bottom_of(mid);
            // TODO: some function to REMOVE LayoutItems from each other...
        }

        THEN("children that were not also children of the LayoutItem are moved into its place")
        {
            REQUIRE(left->m_znode->m_parent == root->m_znode.get());
        }
    }

    /*           A                               A
     *            \                               \
     *             +---+                           +---+---+
     *              \   \                           \   \   \
     *               B   C                           E   F   C
     *                \              (del B) =>           \
     *                 +--+--+                             G
     *                  \  \  \
     *                   D  E  F
     *                          \
     *                           G
     */
    WHEN("the parent of explicitly placed ZNodes is deleted because their LayoutItem is deleted")
    {
        std::shared_ptr<LayoutRoot> root;
        std::vector<ZNode*> raw_pointers;
        {
            Handle root_handle = Application::get_instance().get_object_manager()._get_next_handle();
            root = LayoutRoot::create(root_handle, {});
            std::shared_ptr<StackLayout> a = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
            std::shared_ptr<StackLayout> b = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
            std::shared_ptr<StackLayout> c = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
            std::shared_ptr<StackLayout> d = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
            std::shared_ptr<StackLayout> e = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
            std::shared_ptr<StackLayout> f = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);
            std::shared_ptr<StackLayout> g = StackLayout::create(STACK_DIRECTION::LEFT_TO_RIGHT);

            raw_pointers = {
                a->m_znode.get(),
                b->m_znode.get(),
                c->m_znode.get(),
                d->m_znode.get(),
                e->m_znode.get(),
                f->m_znode.get(),
                g->m_znode.get(),
            };

            root->set_item(a);
            a->add_item(b);
            a->add_item(c);
            b->add_item(d);
            a->add_item(e);
            a->add_item(f);
            f->add_item(g);

            e->place_above(d);
            f->place_on_top_of(b);

            std::vector<ZNode*> expected = {
                raw_pointers[0], // a
                raw_pointers[1], // b
                raw_pointers[3], // d
                raw_pointers[4], // e
                raw_pointers[5], // f
                raw_pointers[6], // g
                raw_pointers[2], // c
            };
            std::vector<ZNode*> flattened = raw_pointers[0]->flatten_hierarchy();
            REQUIRE(flattened.size() == expected.size());
            for (size_t i = 0; i < flattened.size(); ++i) {
                REQUIRE(flattened[i] == expected[i]);
            }

            a->_remove_child(b->get_handle());
        }

        THEN("the explicitly placed ZNodes moves into its place")
        {
            std::vector<ZNode*> expected = {
                raw_pointers[0], // a
                raw_pointers[4], // e
                raw_pointers[5], // f
                raw_pointers[6], // g
                raw_pointers[2], // c
            };
            std::vector<ZNode*> flattened = raw_pointers[0]->flatten_hierarchy();
            REQUIRE(flattened.size() == expected.size());
            for (size_t i = 0; i < flattened.size(); ++i) {
                REQUIRE(flattened[i] == expected[i]);
            }
        }
    }
}
