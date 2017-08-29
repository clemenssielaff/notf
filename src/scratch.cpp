#if 0
#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>


struct Capability;
using CapabilityPtr = std::shared_ptr<Capability>;

/**********************************************************************************************************************/

/** Baseclass for all Widget capabilities.
 * Used so that we can have common pointer type.
 */
struct Capability {
protected:
    Capability() = default; // you shouldn't instantiate any non-derived Capabilities
};

/**********************************************************************************************************************/

/**
 *
 */
class CapabilityMap { // TODO: maybe replace internal Capability map with a vector?

public: // methods ****************************************************************************************************/
    /** Returns a requested capability by type.
     * If the map does not contain the requested capability, an empty pointer is returned.
     */
    template <typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    std::shared_ptr<CAPABILITY> get()
    {
        try {
            return std::static_pointer_cast<CAPABILITY>(m_capabilities.at(typeid(CAPABILITY).hash_code()));
        } catch (const std::out_of_range&) {
            return {};
        }
    }

    /** Returns a requested capability by type.
     * If the map does not contain the requested capability, an empty pointer is returned.
     */
    template <typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    const CapabilityPtr get() const { return const_cast<CapabilityMap*>(this)->get<CAPABILITY>(); }

    /** Inserts or replaces a capability in the map.
     * @param capability    Capability to insert.
     */
    template <typename CAPABILITY, ENABLE_IF_SUBCLASS(CAPABILITY, Capability)>
    void insert(std::shared_ptr<CAPABILITY> capability)
    {
        size_t a = typeid(CAPABILITY).hash_code();

        m_capabilities.insert({a, std::move(capability)});
    }

private: // fields ****************************************************************************************************/
    std::unordered_map<size_t, CapabilityPtr> m_capabilities;
};

struct TextCap : public Capability {
    std::string text = "Text Capability";
};

struct MarginCap : public Capability {
    std::string text = "Margin Capability";
};

struct Arsch {
    std::string text = "Arsch";
};

int main()
{
    CapabilityMap map;

    auto textcap   = std::make_shared<TextCap>();
    auto margincap = std::make_shared<MarginCap>();
    auto arschcap  = std::make_shared<Arsch>();

    map.insert(textcap);
    map.insert(margincap);
    //    map.insert(arschcap);

    auto result = map.get<MarginCap>();

    if (result) {
        std::cout << "I've got a: " << result->text << std::endl;
    }
    else {
        std::cout << "I've got a: NOPE" << std::endl;
    }

    return 0;
}
#endif
