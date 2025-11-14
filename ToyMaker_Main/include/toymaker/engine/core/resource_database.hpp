/**
 * @ingroup ToyMakerResourceDB
 * @file core/resource_database.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Headers relating to resources and their management for a given project. 
 * 
 * If you are reading this, then the class you are probably most interested in is ToyMaker::ResourceDatabase.
 * 
 * @version 0.3.2
 * @date 2025-08-31
 * 
 * @see ToyMaker::ResourceDatabase
 * @see ToyMaker::ResourceConstructor
 * @see ToyMaker::Resource
 */

/**
 * @defgroup ToyMakerResourceDB Resource Database
 * @ingroup ToyMakerEngine ToyMakerSerialization ToyMakerCore
 * 
 */

#ifndef FOOLSENGINE_RESOURCEDATABASE_H
#define FOOLSENGINE_RESOURCEDATABASE_H

#include <memory>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <iostream>
#include <type_traits>

#include <nlohmann/json.hpp>

#include "../registrator.hpp"

namespace ToyMaker {
    class IResource;
    class IResourceFactory;
    class IResourceConstructor;

    class ResourceDatabase;
    template <typename TResource> class ResourceFactory;
    template <typename TResource> class Resource;
    template <typename TResource, typename TMethod> class ResourceConstructor;

    /**
     * @ingroup ToyMakerResourceDB
     *
     * @brief Base class of all Resource types.
     * 
     */
    class IResource {
    public:
        /**
         * @brief Destroy the IResource object
         * 
         */
        virtual ~IResource()=default;

        /**
         * @brief Get the Resource Type string for this resource.
         * 
         * @return std::string The resource type string for this Resource.
         */
        virtual std::string getResourceTypeName_() const=0;
    protected:

        /**
         * @brief Construct a new IResource object
         * 
         */
        IResource()=default;

        /**
         * @brief Registers this resource as a Resource type with the ResourceDatabase.
         * 
         * @tparam TResource The type of Resource being registered.
         */
        template <typename TResource>
        static void RegisterResource();
    private:
    };

    /**
     * @ingroup ToyMakerResourceDB
     * @brief A class that holds references to all constructors for a type of Resource object.
     * 
     */
    class IResourceFactory {
    public:

        /**
         * @brief Destroy the IResourceFactory object
         * 
         */
        virtual ~IResourceFactory()=default;

        /**
         * @brief Creates a resource object of a specific type using its JSON resource description.
         * 
         * @param resourceDescription A description of the resource.
         * @return std::shared_ptr<IResource> A reference to the constructed resource.
         */
        virtual std::shared_ptr<IResource> createResource(const nlohmann::json& resourceDescription)=0;
    protected:

        /**
         * @brief A list of constructors that may be used to create a Resource of this kind.
         * 
         */
        std::map<std::string, std::unique_ptr<IResourceConstructor>> mFactoryMethods {};
    private:
    friend class ResourceDatabase;
    };

    /**
     * @ingroup ToyMakerResourceDB
     * @brief A single way that a resource may be constructed.
     * 
     */
    class IResourceConstructor {
    public:
        /**
         * @brief Get the Resource Constructor type string for this constructor.
         * 
         * @return std::string The resource constructor type string.
         */
        virtual std::string getResourceConstructorName_() const=0;

        /**
         * @brief Creates a resource object using the parameters specified in methodParameters.
         * 
         * @param methodParameters The parameters used to construct an object via this constructor.
         * @return std::shared_ptr<IResource> A reference to the created resource.
         */
        virtual std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters)=0;

        /**
         * @brief Destroys this resource constructor (when the application is terminated.)
         * 
         */
        virtual ~IResourceConstructor()=default;

    protected:

        /**
         * @brief Construct a new IResourceConstructor object
         * 
         */
        IResourceConstructor()=default;

        /**
         * @brief Registers this resource constructor against its respective ResourceFactory during static initialization.
         * 
         * @tparam TResource The type of Resource this constructor creates.
         * @tparam TResourceConstructor The constructor which derives this class.
         */
        template <typename TResource, typename TResourceConstructor>
        static void RegisterResourceConstructor();
    private:
    friend class ResourceDatabase;
    };

    /**
     * @ingroup ToyMakerResourceDB
     * @brief A database of all Resource types available for this project, and the various ResourceConstructors responsible for making them.
     * 
     * Also holds a list of resource definitions.  When an attempt is made to retrieve a resource by its name, a copy of a reference to it is returned if it exists in memory, or the resource is constructed per the parameters stored in its description.
     * 
     * ## Usage:
     * 
     * A resource description of a known resource might look something like this:
     * 
     * ```c++
     * nlohmann::json buttonPressedTextureDescription = "{\
     *     \"name\": \"Bad_Button_Pressed_Texture\",\
     *     \"type\": \"Texture\",\
     *     \"method\": \"fromFile\",\
     *     \"parameters\": {\
     *         \"path\": \"data/textures/button_pressed.png\"\
     *     }\
     * }";
     * ```
     * 
     * This description can then be stored in the ResourceDatabase, like so:
     * 
     * ```c++
     * ToyMaker::ResourceDatabase::AddResourceDescription(buttonPressedTextureDescription);
     * ```
     * 

     * 
     * When the resource is required later on it can be created or retrieved, like this:
     * 
     * ```c++
     * std::shared_ptr<ToyMaker::Texture> buttonPressedTexture { ToyMaker::ResourceDatabase::GetRegisteredResource(\"Bad_Button_Pressed_Texture\") };
     * ```
     * 
     * The resource will be constructed based on its type (`"type": "Texture"`), constructor (`"method": "fromFile"`), and parameters (`"parameters": {"path": "data/textures/button_pressed.png"}`).
     * 
     *
     * @warning 
     * Attempting to store the two Resource descriptions with the same name is invalid, and will cause an error.
     * 
     * @warning
     * The ResourceDatabase does not check whether the construction parameters provided will actually help create a valid Resource object.  It only checks that the Resource and Resource Constructor specified in the description are known to it.
     * 
     * @warning
     * Invalid parameters will likely cause an error to be thrown, depending on the ResourceConstructor's implementation.
     * 
     * @see Resource
     * @see ResourceConstructor
     */
    class ResourceDatabase {
    public:
        /**
         * @brief Returns the sole instance of the ResourceDatabase used by this application.
         * 
         * @return ResourceDatabase& A reference to this project's ResourceDatabase.
         */
        static ResourceDatabase& GetInstance();

        /**
         * @brief Gets a reference to or constructs a Resource by name whose description has been stored in this ResourceDatabase.
         * 
         * @tparam TResource The type of resource being retrieved.
         * @param resourceName The name of the resource being retrieved.
         * @return std::shared_ptr<TResource> A reference to the retrieved resource.
         */
        template <typename TResource>
        static std::shared_ptr<TResource> GetRegisteredResource(const std::string& resourceName);

        /**
         * @brief Constructs a fresh anonymous resource using a registered constructor according to its description in JSON.
         * 
         * @tparam TResource The type of Resource being constructed.
         * @param resourceDescription A description of the resource, including parameters used by its factory in its construction.
         * @return std::shared_ptr<TResource> A reference to the newly constructed resource.
         */
        template <typename TResource>
        static std::shared_ptr<TResource> ConstructAnonymousResource(const nlohmann::json& resourceDescription);

        /**
         * @brief Tests whether the description of a particular named resource exists in this project's ResourceDatabase.
         * 
         * @param resourceName The name of the resource whose registration is being tested.
         * @retval true If a description for this Resource exists;
         * @retval false If no description for this Resource exists;
         */
        static bool HasResourceDescription(const std::string& resourceName);

        /**
         * @brief Tests whether a particular resource has already been loaded into memory.
         * 
         * @tparam TResource The type of resource whose existence is being tested.
         * @param resourceName The name of the resource.
         * @retval true If the resource is in memory;
         * @retval false If the resource isn't in memory;
         */
        template <typename TResource>
        static bool HasResource(const std::string& resourceName);

        /**
         * @brief Registers a factory responsible for tracking valid ResourceConstructors for a particular type of Resource.
         * 
         * @tparam TResource The type of Resource the factory will manage.
         * @param factoryName The resource type string of the resource.
         * @param pFactory A reference to the factory to be stored by this project's ResourceDatabase.
         */
        template <typename TResource>
        void registerFactory (const std::string& factoryName, std::unique_ptr<IResourceFactory> pFactory);

        /**
         * @brief Registers a particular ResourceConstructor for a Resource
         * 
         * @tparam TResource The type of resource constructed.
         * @tparam TResourceConstructor The resource constructor's type.
         * @param resourceType The resource type string.
         * @param methodName The resource constructor type string.
         * @param pFactoryMethod A pointer to the constructor.
         */
        template <typename TResource, typename TResourceConstructor>
        void registerResourceConstructor (const std::string& resourceType, const std::string& methodName, std::unique_ptr<IResourceConstructor> pFactoryMethod);

        /**
         * @brief Adds the description (name, constructor, related parameters) to the resource database.
         * 
         * A resource stored in this way may be retrieved at any point later in the project using GetRegisteredResource()
         * 
         * @param resourceDescription A description of the Resource.
         * 
         * @see GetRegisteredResource()
         */
        static void AddResourceDescription (const nlohmann::json& resourceDescription);


    private:
        /**
         * @brief Asserts the correctness of a resource's description, per the Resource types and constructors known to the ResourceDatabase.
         * 
         * @param resourceDescription The description of a resource.
         */
        static void AssertResourceDescriptionValidity(const nlohmann::json& resourceDescription);

        /**
         * @brief Pointers to different factories, each responsible for the creation of different kinds of resources.
         * 
         * Each factory tracks all constructors that can be used to create a resource.
         * 
         */
        std::map<std::string, std::unique_ptr<IResourceFactory>> mFactories {};

        /**
         * @brief References to all resources that have been loaded and are being held in memory.
         * 
         */
        std::map<std::string, std::weak_ptr<IResource>> mResources {};

        /**
         * @brief A list of all Resources that are known to the database, including details of how they may be constructed if no instances of them are present in memory.
         * 
         */
        std::map<std::string, nlohmann::json> mResourceDescriptions {};

        /**
         * @brief Construct a new ResourceDatabase object.
         * 
         */
        ResourceDatabase() = default;
    };

    /**
     * @ingroup ToyMakerResourceDB
     * @brief The base class for any type whose creation and storage should be managed by the ResourceDatabase.
     * 
     * References to a Resource are ultimately returned as shared pointers the the immediate subclass of this class.
     * 
     * ## Usage:
     * 
     * A class that wishes to be seen by the ResourceDatabase should have the following signature:
     * 
     * ```c++
     * class Texture : public Resource<Texture> { // NOTE: TDerived derives from Resource<TDerived>
     * public:
     *     // NOTE: Must have static function that names the resource
     *     inline static std::string getResourceTypeName() { return "Texture"; }
     * 
     *     Texture::Texture(
     *         GLuint textureID,
     *         const ColorBufferDefinition& colorBufferDefinition,
     *         const std::string& filepath
     *     ) : 
     *     Resource<Texture>{0},  // NOTE: Explicit call to Resource<TDerived>'s constructor
     *     mID { textureID }, 
     *     mFilepath { filepath },
     *     mColorBufferDefinition{colorBufferDefinition}
     *     {}
     *
     *     // ...
     *     // ... The rest of the class' definition
     *     // ...
     * };
     * ```
     * 
     * @todo Determine whether it is actually necessary to have all a Resource's construction and assignment operators defined when using the ResourceDatabase for something.
     * 
     * @tparam TDerived The class that wishes to be exposed to ResourceDatabase. 
     */
    template <typename TDerived>
    class Resource: public IResource {
    public:
        /**
         * @brief Get the resource type string for this resource.
         * 
         * @remark Requires the derived type to implement a public static TDerived::getResourceTypeName() function.
         * 
         * @return std::string The resource type string for this resource.
         */
        inline std::string getResourceTypeName_() const override { return TDerived::getResourceTypeName(); }

    protected:
        /**
         * @brief Construct a new resource object
         * 
         * @param explicitlyInitializeMe A dummy value which should have been removed long ago when the author discovered the `explicit` keyword.
         */
        explicit Resource(int explicitlyInitializeMe) { (void)explicitlyInitializeMe/* prevent unused parameter warnings */; s_registrator.emptyFunc();/* Ensures that the registrator for this resource type is not optimized out (I think) */ }
    private:

        /**
         * @brief A helper function that actually performs the task of registering this class with the ResourceDatabase.
         * 
         * This registration occurs during the static initialization phase of the program.
         * 
         */
        static void registerSelf();

        /**
         * @brief A static variable to this classes Registrator.
         * 
         * The registrator ensures that registerSelf() is called during the program's static initialization.
         * 
         * @see Registrator<T>
         * 
         */
        inline static Registrator<Resource<TDerived>>& s_registrator { Registrator<Resource<TDerived>>::getRegistrator() };

    friend class Registrator<Resource<TDerived>>;
    };

    /**
     * @ingroup ToyMakerResourceDB
     * @brief Tracks pointers to all ResourceConstructors responsible for creating a resource of one type.
     * 
     * An object of this kind is created automatically when a resource type registers itself for management by the ResourceDatabase.
     * 
     * @tparam TResource The resource this factory can create.
     */
    template<typename TResource> 
    class ResourceFactory: public IResourceFactory {
    public:
        /**
         * @brief Construct a new Resource Factory object
         */
        ResourceFactory() { /* IMPORTANT: do not remove */ }
    private:
        /**
         * @brief Create an object of type Resource, specifying its constructor and the constructor's parameters in JSON.
         * 
         * @param resourceDescription A JSON description containing the resource constructor's name and parameters.
         * 
         * @return std::shared_ptr<IResource> A shared pointer to the base class of the Resource.
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& resourceDescription) override;
    };

    /**
     * @ingroup ToyMakerResourceDB
     * @brief An object representing one method for creating a resource of a given kind.
     * 
     * A single resource may be created in multiple ways.  A texture resource, for example, may be loaded from an image file, or procedurally generated, or simply allocated.
     * 
     * Each method that results in a Texture implements ResourceConstructor, and is recorded in the Resource's ResourceFactory, which is in turn used by a ResourceDatabase.
     * 
     * ## Usage
     * 
     * An example of a ResourceConstructor definition is given below:
     * 
     * ```c++
     * // NOTE: TConstructor is derived from ResourceConstructor<TResource, TConstructor>
     * class NineSlicePanelFromDescription: public ToyMaker::ResourceConstructor<NineSlicePanel, NineSlicePanelFromDescription> { 
     * public:
     *     // NOTE: explicit call to the ResourceConstructor base class' constructor
     *     NineSlicePanelFromDescription(): ToyMaker::ResourceConstructor<NineSlicePanel, NineSlicePanelFromDescription> {0} {} 
     *     // NOTE: static method returning the name of the resource constructor
     *     inline static std::string getResourceConstructorName() { return "fromDescription"; } 
     * 
     * private:
     *     // NOTE: The actual method responsible for constructing an instance of the resource
     *     std::shared_ptr<ToyMaker::IResource> createResource(const nlohmann::json& methodParameters) override;
     * };
     * ```
     * 
     * @tparam TResource The type of resource this constructor creates.
     * @tparam TResourceFactoryMethod The derived class, the resource constructor itself.
     * 
     * @see Resource
     * @see ResourceFactory
     * @see ResourceDatabase
     */
    template<typename TResource, typename TResourceFactoryMethod>
    class ResourceConstructor: public IResourceConstructor {
    public:
        /**
         * @brief Gets the resource constructor type string of the constructor.
         * 
         * @remarks requires static std::string TResourceFactoryMethod::getResourceConstructorName() to be defined.
         * 
         * @return std::string The resource constructor's type string.
         */
        inline std::string getResourceConstructorName_() const override { return TResourceFactoryMethod::getResourceConstructorName(); }

    protected:
        /**
         * @brief Construct a new ResourceConstructor object
         * 
         * @param explicitlyInitializeMe A dummy parameter that should have been removed a long time ago when the author discovered `explicit`
         */
        explicit ResourceConstructor(int explicitlyInitializeMe) {
            (void)explicitlyInitializeMe; // prevent unused parameter warnings
            s_registrator.emptyFunc(); // required to prevent this class' registrator object from being optimized away
        }

    private:
        /**
         * @brief A helper function that informs the ResourceDatabase that this constructor type exists.
         * 
         */
        static void registerSelf();

        /**
         * @brief A helper class responsible for calling this class' registerSelf() method at during the static initialization phase of a program.
         * 
         * @see Registrator<T>
         */
        inline static Registrator<ResourceConstructor<TResource, TResourceFactoryMethod>> s_registrator {
            Registrator<ResourceConstructor<TResource, TResourceFactoryMethod>>::getRegistrator()
        };

    friend class Registrator<ResourceConstructor<TResource, TResourceFactoryMethod>>;
    friend class ResourceFactory<TResource>;
    };

    // ##########################################################################################
    // FUNCTION DEFINITIONS
    // ##########################################################################################

    template <typename TResource>
    std::shared_ptr<TResource> ResourceDatabase::GetRegisteredResource(const std::string& resourceName) {
        ResourceDatabase& resourceDatabase { ResourceDatabase::GetInstance() };
        std::shared_ptr<IResource> pResource { nullptr };

        // Search known resources first and validate it against the requested type
        std::map<std::string, nlohmann::json>::iterator resourceDescPair { resourceDatabase.mResourceDescriptions.find(resourceName) };
        assert(
            resourceDescPair != resourceDatabase.mResourceDescriptions.end() 
            && "No resource with this name was found amongst known resources"
        );
        assert(
            resourceDescPair->second["type"].get<std::string>() == TResource::getResourceTypeName()
            && "The type of resource requested does not match the type of resource as declared \
            in its description"
        );

        // Look through resources currently in memory
        {
            std::map<std::string, std::weak_ptr<IResource>>::iterator resourcePair { resourceDatabase.mResources.find(resourceName) };
            if(resourcePair != resourceDatabase.mResources.end()) {
                // If a corresponding entry was found but the resource itself had already been moved
                // out of memory, remove the entry
                if(resourcePair->second.expired()) {
                    resourceDatabase.mResources.erase(resourcePair->first);

                // We got lucky, and this resource is still in memory
                } else {
                    pResource = resourcePair->second.lock();
                }
            }
        }

        // If we still haven't got an in-memory copy, construct this resource using its description,
        // as registered in our resource description table 
        if(!pResource) {
            pResource = std::static_pointer_cast<typename Resource<TResource>::IResource, TResource>(
                ResourceDatabase::ConstructAnonymousResource<TResource>(resourceDescPair->second)
            );
            resourceDatabase.mResources[resourceName] = std::weak_ptr<IResource>{ pResource };
        }

        // Finally, cast it down to the requested type and hand it over to whoever asked
        // 
        // TODO: how can we prevent unwanted modification of the resource by one of its 
        // users?
        return std::static_pointer_cast<TResource>(std::static_pointer_cast<Resource<TResource>>(pResource));
    }

    template<typename TResource>
    std::shared_ptr<TResource> ResourceDatabase::ConstructAnonymousResource(const nlohmann::json& resourceDescription) {
        ResourceDatabase& resourceDatabase { ResourceDatabase::GetInstance() };
        std::shared_ptr<IResource> pResource { nullptr };

        AssertResourceDescriptionValidity(resourceDescription);

        // Construct this resource using its description
        pResource = resourceDatabase
            .mFactories.at(resourceDescription.at("type").get<std::string>())
            ->createResource(resourceDescription);
        assert(pResource && "Resource could not be constructed");

        // Finally, cast it down to the requested type and hand it over to whoever asked
        // 
        // TODO: how can we prevent unwanted modification of the resource by one of its 
        // users?
        return std::static_pointer_cast<TResource>(std::static_pointer_cast<Resource<TResource>>(pResource));
    }

    template<typename TResource>
    bool ResourceDatabase::HasResource(const std::string& resourceName) {
        ResourceDatabase& resourceDatabase { GetInstance() };
        bool descriptionPresent { HasResourceDescription(resourceName) };
        bool typeMatched { false };
        bool objectLoaded { false };
        if(descriptionPresent){
            typeMatched = (
                TResource::getResourceTypeName() 
                == resourceDatabase.mResourceDescriptions.at(resourceName).at("type").get<std::string>()
            );
            objectLoaded = (
                resourceDatabase.mResources.find(resourceName)
                != resourceDatabase.mResources.end()
            );
        }
        return (
            descriptionPresent && typeMatched && objectLoaded
        );
    }

    template <typename TResource>
    std::shared_ptr<IResource> ResourceFactory<TResource>::createResource(const nlohmann::json& resourceDescription) {
        std::cout << "Loading resource (" << TResource::getResourceTypeName() << ") : " << nlohmann::to_string(resourceDescription["parameters"]) << "\n";
        return mFactoryMethods.at(resourceDescription["method"].get<std::string>())->createResource(
            resourceDescription["parameters"].get<nlohmann::json>()
        );
    }

    template <typename TDerived>
    void Resource<TDerived>::registerSelf() {
        IResource::RegisterResource<TDerived>();
    }

    template <typename TResource, typename TResourceFactoryMethod>
    void ResourceConstructor<TResource, TResourceFactoryMethod>::registerSelf() {
        // ensure that the associated factory is registered before methods are added to it
        Registrator<Resource<TResource>>& resourceRegistrator { Registrator<Resource<TResource>>::getRegistrator() };
        resourceRegistrator.emptyFunc();
        // actually register this method now
        IResourceConstructor::RegisterResourceConstructor<TResource, TResourceFactoryMethod>();
    }

    template <typename TResource>
    void IResource::RegisterResource() {
        ResourceDatabase::GetInstance().registerFactory<TResource>(TResource::getResourceTypeName(), std::make_unique<ResourceFactory<TResource>>());
    }

    template <typename TResource, typename TResourceConstructor>
    void IResourceConstructor::RegisterResourceConstructor() {
        ResourceDatabase::GetInstance().registerResourceConstructor<TResource, TResourceConstructor>(TResource::getResourceTypeName(), TResourceConstructor::getResourceConstructorName(), std::make_unique<TResourceConstructor>());
    }

    template <typename TResource>
    void ResourceDatabase::registerFactory(const std::string& factoryName, std::unique_ptr<IResourceFactory> pFactory) {
        assert((std::is_base_of<IResource, TResource>::value) && "Resource must be subclass of IResource");
        mFactories[factoryName] = std::move(pFactory);
    }

    template <typename TResource, typename TResourceConstructor>
    void ResourceDatabase::registerResourceConstructor(const std::string& resourceType, const std::string& methodName, std::unique_ptr<IResourceConstructor> pFactoryMethod) {
        assert((std::is_base_of<IResource, TResource>::value) && "Resource must be subclass of IResource");
        assert((std::is_base_of<IResourceConstructor, TResourceConstructor>::value) && "Resource must be subclass of IResourceConstructor");
        mFactories.at(resourceType)->mFactoryMethods[methodName] = std::move(pFactoryMethod);
    }

}

#endif
