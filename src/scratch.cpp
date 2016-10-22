#if 0
#include <iostream>

#include "core/layout.hpp"

using namespace notf;

class MyLayoutItem : public LayoutItem {

    friend class MyLayout;

protected: // methods for MyLayout
    explicit MyLayoutItem()
        : LayoutItem()
    {
    }
};

class MyLayout : public Layout<MyLayoutItem> {

public: // static methods
    /// \brief Factory function to create a new Layout.
    /// \param handle   Handle of this Layout.
    static std::shared_ptr<MyLayout> create(Handle handle = BAD_HANDLE) { return create_item<MyLayout>(handle); }

public:
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2&) const override { return {}; }

    virtual void redraw() override{};

    void fill()
    {

        for (auto i = 0; i < 10; ++i) {
            m_items.push_back({});
        }
    }

    void print()
    {
        std::cout << m_items.size() << std::endl;
    }

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit MyLayout(const Handle handle)
        : Layout<MyLayoutItem>(handle)
    {
    }
};

int main(void)
{
    auto layout = MyLayout::create();
    layout->fill();
    layout->print();
}

#endif
