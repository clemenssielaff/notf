#include <iostream>
#include <memory>

struct Base {
    virtual ~Base() = default;
    virtual void do_blub() const { std::cout << "Base blub"; }
};

struct Deriv : public Base {
    virtual void do_blub() const { std::cout << "Deriv blub"; }
};

struct Third : public Deriv {};

struct BaseIngestor {
    virtual ~BaseIngestor() = default;
    virtual void print_blub(std::shared_ptr<Base>) const { std::cout << "Base ingestor blub" << '\n'; }
};

struct DerivIngestor : public BaseIngestor {
    void print_blub(std::shared_ptr<Base> blubable) const override
    {
        std::cout << "Deriv ingestor, base overload: ";
        blubable->do_blub();
        std::cout << '\n';
    }
    template<class T, class = std::enable_if_t<!std::is_same_v<T, std::shared_ptr<Base>>>>
    void print_blub(T blubable) const
    {
        std::cout << "Deriv ingestor, deriv overload: ";
        blubable->do_blub();
        std::cout << '\n';
    }
};

int main()
{
    auto base = std::make_shared<Base>();
    auto deriv = std::make_shared<Deriv>();
    auto third = std::make_shared<Third>();

    auto base_ingestor = std::make_shared<BaseIngestor>();
    auto deriv_ingestor = std::make_shared<DerivIngestor>();

    deriv_ingestor->print_blub(base);
    deriv_ingestor->print_blub(third);

    return 0;
}
