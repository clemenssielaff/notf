#include "app/core/property_manager.hpp"

#include <algorithm>

#include "common/log.hpp"

namespace {
NOTF_USING_NAMESPACE

/// Instead of copying multiple fields to each handler, we just put them into a "kit" and pass a single reference.
struct HandlerKit {
    PropertyGraph& graph;
    PropertyId property;
    Time time;
};

struct AddPropertyHandler {
    template<typename value_t>
    void operator()(value_t value) const
    {
        m.graph.add_property<value_t>(m.property);
        m.graph.set_property<value_t>(m.property, std::forward<value_t>(value), m.time);
    }
    HandlerKit& m;
};

struct SetPropertyHandler {
    template<typename value_t>
    void operator()(value_t value) const
    {
        m.graph.set_property<value_t>(m.property, std::forward<value_t>(value), m.time);
    }
    HandlerKit& m;
};

struct SetExpressionHandler {
    template<typename value_t>
    void operator()(std::function<value_t(const PropertyGraph&)> expression) const
    {
        m.graph.set_expression<value_t>(m.property, std::move(expression), dependencies, m.time);
    }
    HandlerKit& m;
    std::vector<PropertyId> dependencies;
};

} // namespace

NOTF_OPEN_NAMESPACE

void PropertyManager::execute_batches()
{
    struct CommandHandler {

        void operator()(const Command::Empty&) const {}

        void operator()(const Command::Create& command) const
        {
            std::visit(AddPropertyHandler{m}, std::move(command.value));
        }

        void operator()(const Command::SetValue& command) const
        {
            std::visit(SetPropertyHandler{m}, std::move(command.value));
        }

        void operator()(const Command::SetExpression& command) const
        {
            try {
                std::visit(SetExpressionHandler{m, std::move(command.dependencies)}, std::move(command.expression));
            }
            catch (const property_cyclic_dependency_error&) {
            }
        }

        void operator()(const Command::Delete&) const { m.graph.delete_property(m.property); }

        HandlerKit& m;
    };

    //----------------------------------------------------------------------------------------------------------------//

    { // load batches from the async queue until it is empty
        m_ready_queue.clear();
        InternalBatch batch;
        while (m_async_queue.pop(batch)) {
            m_ready_queue.emplace_back(std::move(batch));
        }
    }

    // sort the batches by time
    std::sort(m_ready_queue.begin(), m_ready_queue.end(),
              [](const InternalBatch& a, const InternalBatch& b) -> bool { return a.time < b.time; });

    // prepare the "kit" that will be used as member fields by the variant visitors.
    HandlerKit kit{m_graph, PropertyId::invalid(), Time::invalid()};

    // execute each command in each batch in order
    for (const InternalBatch& batch : m_ready_queue) {
        kit.time = batch.time;
        for (const Command& command : batch.commands) {
            kit.property = command.property;
            std::visit(CommandHandler{kit}, command.type);
        }
    }
    m_ready_queue.clear();
}

NOTF_CLOSE_NAMESPACE
