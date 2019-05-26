#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <algorithm>

namespace Pack {

using word_t = size_t;

struct Object {

    // use the first 2 bits to indicate the type:
    // number, string, list or bool.
    // all other bits should be set to zero. If they aren't, this is a map of the given size.
    // even with a word size of 8 bits, this still leaves 6 bits (=64) entries in a map. That's quite enough I'd say
    // or we could take the first 3 bits, leave 32 entries in a map (still okay) and be able to differentiate:
    // int8 / bool, int16, int32, int64, float, double, string, list and maps...hmmm

    enum class Type : word_t { MAP = std::numeric_limits<word_t>::max() - 3, LIST, STRING, NUMBER };

    Object(Type type) : type(type) {}

    const Type type;
    std::string name;
    std::vector<Object> schema;
};

struct Map : Object {
    Map(std::initializer_list<std::pair<std::string, Object>> entries) : Object(Type::MAP) {
        schema.reserve(entries.size());
        std::transform(entries.begin(), entries.end(), std::back_inserter(schema),
                       [](std::pair<std::string, Object> entry) {
                           entry.second.name = std::move(entry.first);
                           return entry.second;
                       });
    }
};

struct List : Object {
    List(Object entry) : Object(Type::LIST) { schema.emplace_back(std::move(entry)); }
};

static const Object String(Object::Type::STRING);
static const Object Number(Object::Type::NUMBER);

} // namespace Pack

struct Schema {

    using word_t = Pack::word_t;

    Schema(const Pack::Object& obj) {
        std::vector<word_t> buffer;
        _append_next(obj, buffer);
        description = std::make_shared<typename decltype(description)::element_type>(std::move(buffer));
    }

    std::shared_ptr<const std::vector<word_t>> description;

private:
    word_t _append_next(const Pack::Object& obj, std::vector<word_t>& out) {
        switch (obj.type) {

        // map appends 2+n words, map type identifier / map size / entry schema location for each child schema
        case Pack::Object::Type::MAP: {
            const word_t location = out.size();
            out.push_back(static_cast<word_t>(Pack::Object::Type::MAP));
            out.push_back(obj.schema.size());
            word_t itr = out.size();
            for (size_t i = 0; i < obj.schema.size(); ++i) {
                out.push_back(0);
            }
            for (const Pack::Object& child : obj.schema) {
                out[itr++] = _append_next(child, out);
            }
            return location;
        };

        // list appends one word: list type identifier
        case Pack::Object::Type::LIST: {
            const word_t location = out.size();
            out.push_back(static_cast<word_t>(Pack::Object::Type::LIST));
            _append_next(obj.schema.at(0), out);
            return location;
        }

        // string appends one word: string type identifier
        case Pack::Object::Type::STRING:
            return static_cast<word_t>(Pack::Object::Type::STRING);

        // number appends one word: number type identifier
        case Pack::Object::Type::NUMBER:
            return static_cast<word_t>(Pack::Object::Type::NUMBER);
        }

        return 0;
    }
};

// using namespace std::literals;

int main() {
    Schema schema(Pack::List(Pack::Map{{"coord",
                                        Pack::Map{
                                            {"x", Pack::Number}, //
                                            {"y", Pack::Number}  //
                                        }},
                                       {"name", Pack::String}}));

    std::map<Pack::word_t, std::string_view> legend = {
        {static_cast<Pack::word_t>(Pack::Object::Type::MAP), "Map"},
        {static_cast<Pack::word_t>(Pack::Object::Type::LIST), "List"},
        {static_cast<Pack::word_t>(Pack::Object::Type::STRING), "String"},
        {static_cast<Pack::word_t>(Pack::Object::Type::NUMBER), "Number"},
    };
    size_t line = 0;
    for(const Pack::word_t& word : *schema.description){
        std::cout << line++ << ": ";
//        if(word >= static_cast<Pack::word_t>(Pack::Object::Type::MAP)){
//            std::cout << "Map(" << (word - static_cast<Pack::word_t>(Pack::Object::Type::MAP)) << ')' << std::endl;
//        }
//        else
            if(legend.count(word)){
            std::cout << legend[word] << std::endl;
        } else {
            std::cout << word << std::endl;
        }

    }
    return 0;
}
