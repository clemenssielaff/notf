#include "app/property_batch.hpp"

NOTF_OPEN_NAMESPACE

//====================================================================================================================//

void PropertyBatch::execute()
{
    PropertyUpdateList effects;
    {
        NOTF_MUTEX_GUARD(PropertyGraph::Access<PropertyBatch>::mutex());

        // verify that all updates will succeed first
        for (const PropertyUpdatePtr& update : m_updates) {
            PropertyBody::Access<PropertyBatch>::validate_update(update->property, update);
        }

        // apply the updates
        for (const PropertyUpdatePtr& update : m_updates) {
            PropertyBody::Access<PropertyBatch>::apply_update(update->property, update, effects);
        }
    }

    // fire off the combined event/s
    PropertyGraph::fire_event(std::move(effects));

    // reset in case the user wants to reuse the batch object
    m_updates.clear();
}

NOTF_CLOSE_NAMESPACE
