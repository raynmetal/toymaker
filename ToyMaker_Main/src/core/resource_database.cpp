#include <nlohmann/json.hpp>

#include "toymaker/engine/core/resource_database.hpp"

using namespace ToyMaker;

void ResourceDatabase::AssertResourceDescriptionValidity(const nlohmann::json& resourceDescription)  {
    ResourceDatabase& resourceDatabase { ResourceDatabase::GetInstance() };

    assert(
        (
            resourceDescription.find("type") != resourceDescription.end()
            && resourceDescription["type"].type() == nlohmann::detail::value_t::string
            && (
                resourceDatabase.mFactories.find(resourceDescription["type"].get<std::string>())
                != resourceDatabase.mFactories.end()
            )
        ) && "Resource description must include a type field filled with a known resource type"
    );
    assert(
        (
            resourceDescription.find("method") != resourceDescription.end()
            && resourceDescription["method"].type() == nlohmann::detail::value_t::string
            && (
                resourceDatabase.mFactories[resourceDescription["type"].get<std::string>()]
                    ->mFactoryMethods.find(resourceDescription["method"].get<std::string>())
                != resourceDatabase.mFactories[resourceDescription["type"].get<std::string>()]
                    ->mFactoryMethods.end()
            ) && "Resource description must include a method field filled with a known resource factory method"
        )
    );
    assert(
        (
            resourceDescription.find("parameters") != resourceDescription.end()
            && resourceDescription["parameters"].type() == nlohmann::detail::value_t::object
        ) && "Resource description must contain a parameters field filled with a json object"
    );

    //TODO: assert that params contains values that the method expects
    //TODO: how do we generate documentation that tells a developer what values are expected?
}

void ResourceDatabase::AddResourceDescription(const nlohmann::json& resourceDescription) {
    ResourceDatabase& resourceDatabase { ResourceDatabase::GetInstance() };

    // validate
    AssertResourceDescriptionValidity(resourceDescription);

    assert(
        (
            resourceDescription.find("name") != resourceDescription.end()
            && resourceDescription["name"].type() == nlohmann::detail::value_t::string
        ) && "Resource description to be registered must include a name field filled with a string"
    );
    assert(
        (
            resourceDatabase.mResourceDescriptions.find(resourceDescription["name"].get<std::string>())
            == resourceDatabase.mResourceDescriptions.end()
        ) && "There is already a resource description matching a resource with this name which cannot be overwritten"
    );

    std::string resourceName { resourceDescription.at("name").get<std::string>() };
    resourceDatabase.mResourceDescriptions[resourceName] = resourceDescription;
}

bool ResourceDatabase::HasResourceDescription(const std::string& resourceName) {
    ResourceDatabase& resourceDatabase { GetInstance() };
    bool descriptionPresent { resourceDatabase.mResourceDescriptions.find(resourceName) != resourceDatabase.mResourceDescriptions.end() };
    return descriptionPresent;
}

ResourceDatabase& ResourceDatabase::GetInstance() {
    static ResourceDatabase resourceDatabase {};
    return resourceDatabase;
}
