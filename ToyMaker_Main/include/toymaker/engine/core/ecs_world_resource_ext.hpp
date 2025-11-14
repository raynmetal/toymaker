/**
 * @file core/ecs_world_resource_ext.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Header file that makes Resource objects useable as ECS components
 * @version 0.3.2
 * @date 2025-08-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef FOOLSENGINE_SIMPLEECSRESOURCECOMPONENTEXT_H
#define FOOLSENGINE_SIMPLEECSRESOURCECOMPONENTEXT_H

#include <type_traits>
#include <nlohmann/json.hpp>

#include "ecs_world.hpp"
#include "resource_database.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerSerialization ToyMakerECSComponent ToyMakerResourceDB
     * @brief Allows a shared pointer to a resource to be constructed as a component for an entity when loading a scene.
     * 
     * The scene loading system, when it comes across such a resource reference in a scene file, will automatically attempt to construct the resource (or fetch a pointer to it, if it has already been constructed) based on its description in the resource database.
     * 
     * The struct itself isn't a "real" object.  It is a container that allows partial specialization for the function within, where partial specialization for functions themselves aren't legal in C++.
     * 
     * @tparam TResource The type of resource being used as a component, contained in a shared pointer
     */
    template <typename TResource>
    struct ComponentFromJSON<std::shared_ptr<TResource>,
        typename std::enable_if<std::is_base_of<
            IResource, TResource
        >::value>::type /**< enable this helper struct only if the type is a subclass of resource */> {

        /**
         * @brief Helper function which constructs or fetches a reference to a resource defined in the Resource Database
         * 
         * @param jsonResource A JSON object holding the type and name of the desired resource
         * @return std::shared_ptr<TResource> A shared pointer to the loaded resource
         */
        static std::shared_ptr<TResource> get(const nlohmann::json& jsonResource) {
            return ResourceDatabase::GetRegisteredResource<TResource>(
                jsonResource.at("resourceName")
            );
        }
    };
}

#endif
