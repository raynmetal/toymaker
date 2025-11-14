/**
 * @ingroup ToyMakerRenderSystem
 * @file model.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Classes, constructors for this engine's representation of 3D models.
 * @version 0.3.2
 * @date 2025-09-04
 * 
 * 
 */

#ifndef FOOLSENGINE_MODEL_H
#define FOOLSENGINE_MODEL_H

#include <vector>
#include <string>
#include <map>
#include <unordered_set>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/material.h>

#include "core/ecs_world_resource_ext.hpp" // this instead of the regular ECS and resource includes
#include "vertex.hpp"
#include "mesh.hpp"
#include "texture.hpp"
#include "shader_program.hpp"
#include "material.hpp"


namespace ToyMaker {

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerECSComponent
     * 
     * @brief This engine's representation of a single unrigged 3D model.
     * 
     * This object:
     * 
     *   - Stores references to all the meshes used by the model
     * 
     *   - Stores the hierarchical relationship between the meshes
     * 
     *   - Stores material properties used by shaders for each mesh
     * 
     * 
     * @see toymaker/core/ecs_world_resource_ext.hpp
     * 
     */
    class StaticModel : public Resource<StaticModel>{
    public:
        /**
         * @brief Gets the resource type string for this object.
         * 
         * @return std::string This type's resource type string.
         */
        inline static std::string getResourceTypeName() { return "StaticModel"; }

        /**
         * @brief Gets the component type string for this object.
         * 
         * @return std::string This type's component type string.
         */
        inline static std::string getComponentTypeName() { return "StaticModel"; }

        /**
         * @brief Constructs a model out of a list of handles to meshes and materials.
         * 
         * Every mesh in the mesh list must have its material reference in the same index of the material list.
         * 
         * @param meshHandles List of meshes making up this model.
         * @param materialHandles The materials corresponding to each mesh.
         * 
         */
        StaticModel(const std::vector<std::shared_ptr<StaticMesh>>& meshHandles, const std::vector<std::shared_ptr<Material>>& materialHandles);

        /** @brief Model destructor */
        ~StaticModel();

        /** @brief Move constructor */
        StaticModel(StaticModel&& other);
        /** @brief Copy constructor */
        StaticModel(const StaticModel& other);

        /** @brief Move assignment */
        StaticModel& operator=(StaticModel&& other);
        /** @brief Copy assignment */
        StaticModel& operator=(const StaticModel& other);

        /**
         * @brief Gets the list of StaticMeshes associated with this Model object.
         * 
         * @return std::vector<std::shared_ptr<StaticMesh>> This model's meshes.
         */
        std::vector<std::shared_ptr<StaticMesh>> getMeshHandles() const;

        /**
         * @brief Gets the materials associated with this model object.
         * 
         * @return std::vector<std::shared_ptr<Material>> This model's materials.
         */
        std::vector<std::shared_ptr<Material>> getMaterialHandles() const;

    private:
        /**
         * @brief The meshes that make up this model
         */
        std::vector<std::shared_ptr<StaticMesh>> mMeshHandles {};
        /**
         * @brief The materials that correspond to each mesh on this model
         * 
         */
        std::vector<std::shared_ptr<Material>> mMaterialHandles {};

        /**
         * @brief Utility method for destroying resources associated with this model
         * 
         */
        void free();

        /**
         * @brief Utility method for taking resources from another instance of this class 
         * 
         */
        void stealResources(StaticModel& other);

        /**
         * @brief Utility method for deeply replicating resources from another instance of this class
         *
         */
        void copyResources(const StaticModel& other);

        /**
         * @brief Destroys the resources associated with this object.
         * 
         */
        void destroyResource();

        /**
         * @brief Releases resources associated with this object so that they may be claimed by another part of the program.
         * 
         */
        void releaseResource();
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief A constructor method for StaticModels that loads such a model from its model file (w/ extensions such as .fbx, .obj, .gltf, and so on)
     * 
     * Such a resource's description in JSON might look like:
     * 
     * ```jsonc
     * {
     *     "method": "fromFile",
     *     "name": "EagleModel_One",
     *     "parameters": {
     *         "path": "data/models/UrEagle.obj",
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
     * ```
     * 
     * Where the `path` property is required and the `material_overrides` property may be left unspecified.  The number next to each material override represents the index of the mesh whose material is being overridden.
     * 
     */
    class StaticModelFromFile: public ResourceConstructor<StaticModel, StaticModelFromFile> {
    public:
        /**
         * @brief Creates a StaticModelFromFile object.
         * 
         */
        StaticModelFromFile();

        /**
         * @brief Gets the resource constructor type string for this constructor.
         * 
         * @return std::string This object's resource constructor type string.
         */
        inline static std::string getResourceConstructorName() { return "fromFile"; }

    private:

        /**
         * @brief Creates a StaticModel resource based on its parameters.
         * 
         * @param methodParameters The parameters associated with this model object.
         * 
         * @return std::shared_ptr<IResource> A reference to the constructed resource.
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };
}

#endif
