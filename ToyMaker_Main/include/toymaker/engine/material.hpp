/**
 * @ingroup ToyMakerRenderSystem
 * @file material.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Functions related to rendering materials.
 * @version 0.3.2
 * @date 2025-09-03
 * 
 * A material is a collection of key value pairs that relate to the rendering system in some way, where each element is some string-type pair declared by the rendering system at the start of the application.
 * 
 * 
 */

#ifndef FOOLSENGINE_MATERIAL_H
#define FOOLSENGINE_MATERIAL_H

#include <vector>
#include <string>
#include <map>
#include <queue>

#include <GL/glew.h>

#include "texture.hpp"
#include "core/resource_database.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A collection of key value pairs used to control the behaviour of the rendering system when rendering a single "object."
     * 
     * Each key-value pair is a string-type pair declared by the rendering system or its submodules at the start of the application.
     * 
     * While different engines implement materials differently, some common uses for them are:
     * 
     *   - Storing a collection of albedo, specular, normal, and displacement textures associated with a renderable object.
     * 
     *   - Specifying overrides for shader programs at particular points in the rendering pipeline.
     * 
     *   - Specifying the type of rendering queue a renderable object should be sent to (opaque vs transparent)
     * 
     *   - Storing the intensity of some post-processing effects (like exposure, gamma)
     * 
     *   - Storing other miscellaneous data related to a rendering pass (say an albedo color multiplier in the geometry pass, or the number of iterations for a blur shader).
     * 
     * @todo Not happy with all the redundant code for getters-setters and material property registrators.  Maybe replace with a templated version somehow?
     * 
     */
    class Material : public Resource<Material> {
    public:
        /**
         * @brief Overrides various material related properties based on a JSON description of the overrides.
         * 
         * Here is an example of material overrides for a model resource defined in a scene file:
         * 
         * ```jsonc
         * 
         * {
         *     "method": "fromFile",
         *     "name": "SwallowModel_One",
         *     "parameters": {
         *         "path": "data/models/UrSwallow.obj",
         *         "material_overrides": {
         *             "0": [
         *                 {
         *                     "name": "colorMultiplier",
         *                     "type": "vec4",
         *                     "value": [0.05, 0.05, 0.05, 1.0]
         *                 }
         *             ],
         *             "1": [
         *                 {
         *                     "name": "colorMultiplier",
         *                     "type": "vec4",
         *                     "value": [0.05, 0.05, 0.05, 1.0]
         *                 }
         *             ]
         *         }
         *     },
         *     "type": "StaticModel"
         * }
         * 
         * ```
         * 
         * 
         * @param materialOverrides The material property overrides for an object in JSON.
         * @param material A reference to the material on which the overrides are applied.
         * @return std::shared_ptr<Material> The material with its properties overridden.
         */
        static std::shared_ptr<Material> ApplyOverrides(const nlohmann::json& materialOverrides, std::shared_ptr<Material> material=std::shared_ptr<Material>(new Material{}));

        /**
         * @brief Destroys the material object.
         * 
         */
        virtual ~Material();

        /**
         * @brief Constructs a new Material object.
         * 
         */
        Material();

        /**
         * @brief Material copy constructor.
         * 
         * @param other Material being copied from.
         */
        Material(const Material& other);

        /**
         * @brief Material move constructor.
         * 
         * @param other Material whose resources are being claimed.
         */
        Material(Material&& other);

        /**
         * @brief Material copy constructor.
         * 
         * @param other Material being copied from.
         * @return Material& A reference to this material after the copy.
         */
        Material& operator=(const Material& other);

        /**
         * @brief Material move constructor.
         * 
         * @param other Material whose resources are being claimed.
         * @return Material& A reference to this material after the copy.
         */
        Material& operator=(Material&& other);
        

        /**
         * @brief Updates a float property.
         * 
         * @param name The name of the property.
         * @param value The override value.
         */
        void updateFloatProperty(const std::string& name, float value);

        /**
         * @brief Gets a float property.
         * 
         * @param name The name of the property.
         * @return float Its value.
         */
        float getFloatProperty(const std::string& name);

        /**
         * @brief Updates an int property.
         * 
         * @param name The name of the property
         * @param value The override value.
         */
        void updateIntProperty(const std::string& name, int value);

        /**
         * @brief Gets an int property.
         * 
         * @param name The name of the property.
         * @return int Its value.
         */
        int getIntProperty(const std::string& name);

        /**
         * @brief Updates a glm::vec2 property.
         * 
         * @param name The name of the property.
         * @param value The override value.
         */
        void updateVec2Property(const std::string& name, const glm::vec2& value);

        /**
         * @brief Gets a glm::vec2 property.
         * 
         * @param name The name of the property.
         * @return glm::vec2 Its value.
         */
        glm::vec2 getVec2Property(const std::string& name);

        /**
         * @brief Updates a glm::vec4 property.
         * 
         * @param name The name of the property.
         * @param value The override value.
         */
        void updateVec4Property(const std::string& name, const glm::vec4& value);

        /**
         * @brief Gets a glm::vec4 property.
         * 
         * @param name The name of the property.
         * @return glm::vec4 Its value.
         */
        glm::vec4 getVec4Property(const std::string& name);

        /**
         * @brief Updates a texture property.
         * 
         * @param name The name of the property.
         * @param value The override value.
         */
        void updateTextureProperty(const std::string& name, std::shared_ptr<Texture> value);

        /**
         * @brief Gets the texture property of an object.
         * 
         * @param name The name of the property.
         * @return std::shared_ptr<Texture> Its value.
         */
        std::shared_ptr<Texture> getTextureProperty(const std::string& name);

        /**
         * @brief Registers a project-wide float property.
         * 
         * @param name The name of the property.
         * @param defaultValue Its initial value.
         */
        static void RegisterFloatProperty(const std::string& name, float defaultValue);

        /**
         * @brief Registers a project-wide int property.
         * 
         * @param name The name of the property.
         * @param defaultValue Its initial value.
         */
        static void RegisterIntProperty(const std::string& name, int defaultValue);

        /**
         * @brief Registers a project-wide glm::vec4 property.
         * 
         * @param name The name of the property.
         * @param defaultValue Its initial value.
         */
        static void RegisterVec4Property(const std::string& name, const glm::vec4& defaultValue);

        /**
         * @brief Registers a project-wide glm::vec2 property.
         * 
         * @param name The name of the property.
         * @param defaultValue Its initial value.
         */
        static void RegisterVec2Property(const std::string& name, const glm::vec2& defaultValue);

        /**
         * @brief Registers a project-wide texture property.
         * 
         * @param name The name of the property.
         * @param defaultValue Its initial value.
         */
        static void RegisterTextureHandleProperty(const std::string& name, std::shared_ptr<Texture> defaultValue);

        /**
         * @brief Gets the resource type string for this object.
         * 
         * @return std::string This object's resource type string.
         */
        inline static std::string getResourceTypeName() { return "Material"; }

        /**
         * @brief Initializes the material system, to be called at the start of the application before material properties are registered.
         * 
         */
        static void Init();

        /**
         * @brief Clears all of this project's material system properties, to be called prior to application shutdown.
         * 
         */
        static void Clear();

    private:

        /**
         * @brief A material instantiated at the start of the application, intended to hold all of the material properties and their default values.
         * 
         * This same material is used to determine whether, when updateXProperty or getXProperty are called, the call is legal.
         * 
         */
        static Material* defaultMaterial;

        /**
         * @brief All float property overrides used in this Material instance.
         * 
         */
        std::map<std::string, float> mFloatProperties {};
        /**
         * @brief All int property overrides used in this Material instance.
         * 
         */
        std::map<std::string, int> mIntProperties {};

        /**
         * @brief All glm::vec4 property overrides used in this Material instance.
         * 
         */
        std::map<std::string, glm::vec4> mVec4Properties {};

        /**
         * @brief All glm::vec2 property overrides used in this Material instance.
         * 
         */
        std::map<std::string, glm::vec2> mVec2Properties {};
        std::map<std::string, std::shared_ptr<Texture>> mTextureProperties {};

        /**
         * @brief Destroys the resources used by this material (say, during destruction).
         * 
         */
        void destroyResource();

        /**
         * @brief Releases the resources used by this material so that another material or part of the program can claim it.
         * 
         */
        void releaseResource();
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief A material resource constructor which converts a material represented in JSON to its Material equivalent.
     * 
     * Such a representation might look like this:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "MyMaterial",
     *     "type": "Material",
     *     "method": "fromDescription",
     * 
     *     "parameters": {
     *         "properties": [
     *             {
     *                 "name": "colorMultiplier",
     *                 "type": "vec4",
     *                 "value": [0.05, 0.05, 0.05, 1.0]
     *             },
     *         ],
     *     }
     * }
     * 
     * ```
     * 
     */
    class MaterialFromDescription: public ResourceConstructor<Material, MaterialFromDescription> {
    public:

        /**
         * @brief Constructs a new MaterialFromDescription object.
         * 
         */
        MaterialFromDescription():
        ResourceConstructor<Material, MaterialFromDescription> {0}
        {}

        /**
         * @brief The resource constructor type string associated with this object.
         * 
         * @return std::string This object's resource constructor type string.
         */
        inline static std::string getResourceConstructorName() { return "fromDescription"; }

    private:

        /**
         * @brief The method actually responsible for constructing a Material from its JSON description.
         * 
         * @param methodParameters The material's description in JSON.
         * @return std::shared_ptr<IResource> A reference to the constructed material.
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };
}

#endif
