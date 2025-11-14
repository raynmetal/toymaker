/**
 * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
 * @file shapegen.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains classes used to construct some common procedurally generated meshes and models.
 * @version 0.3.2
 * @date 2025-09-07
 * 
 * 
 */

#ifndef FOOLSENGINE_SHAPEGEN_H
#define FOOLSENGINE_SHAPEGEN_H

#include <memory>
#include <nlohmann/json.hpp>

#include "core/resource_database.hpp"
#include "mesh.hpp"
#include "model.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerResourceDB
     * @brief Mesh constructor that creates a spherical mesh using a latitude-longitude method.
     * 
     * Example:
     * 
     * ```c++
     * 
     * nlohmann::json sphereLightDescription {
     *     {"name", "sphereLight-10lat-5long"},
     *     {"type", StaticMesh::getResourceTypeName()},
     *     {"method", StaticMeshSphereLatLong::getResourceConstructorName()},
     *     {"parameters", {
     *         {"nLatitudes", 10},
     *         {"nMeridians", 5}
     *     }}
     * }
     * 
     * ```
     * 
     */
    class StaticMeshSphereLatLong: public ResourceConstructor<StaticMesh, StaticMeshSphereLatLong> {
    public:
        StaticMeshSphereLatLong():
        ResourceConstructor<StaticMesh, StaticMeshSphereLatLong>{0}
        {}
        inline static std::string getResourceConstructorName() { return "sphereLatLong"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerResourceDB
     * @brief Constructs a rectangle mesh based on its dimensions.
     * 
     * Example:
     * 
     * ```c++
     * 
     * const nlohmann::json rectangleParameters = { 
     *     {"type", ToyMaker::StaticModel::getResourceTypeName()},
     *     {"method", ToyMaker::StaticModelRectangleDimensions::getResourceConstructorName()},
     *     {"parameters", {
     *         {"width", finalRectangleDimensions.x }, {"height", finalRectangleDimensions.y },
     *         {"flip_texture_y", true},
     *         {"material_properties", nlohmann::json::array()},
     *     }}
     * } 
     * 
     * ```
     */
    class StaticMeshRectangleDimensions: public ResourceConstructor<StaticMesh, StaticMeshRectangleDimensions> {
    public:
        StaticMeshRectangleDimensions():
        ResourceConstructor<StaticMesh, StaticMeshRectangleDimensions>{0}
        {}
        inline static std::string getResourceConstructorName() { return "rectangleDimensions"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief Constructs a cuboid mesh based on its dimensions.
     * 
     * Example:
     * 
     * ```c++
     * 
     * renderSet.second.mSkyboxRenderStage->attachMesh("unitCube", ResourceDatabase::ConstructAnonymousResource<StaticMesh>({
     *     {"type", StaticMesh::getResourceTypeName()},
     *     {"method", StaticMeshCuboidDimensions::getResourceConstructorName()},
     *     {"parameters", {
     *         {"depth", 2.f},
     *         {"width", 2.f},
     *         {"height", 2.f},
     *         {"layout", mSkyboxTexture->getColorBufferDefinition().mCubemapLayout},
     *         {"flip_texture_y", true},
     *     }},
     * }));
     * 
     * ```
     * 
     */
    class StaticMeshCuboidDimensions: public ResourceConstructor<StaticMesh, StaticMeshCuboidDimensions> {
    public:
        StaticMeshCuboidDimensions():
        ResourceConstructor<StaticMesh, StaticMeshCuboidDimensions>{0}
        {}
        inline static std::string getResourceConstructorName() { return "cuboidDimensions"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief Constructs a sphere model using the latitude-longitude method.
     * 
     * @see StaticMeshSphereLatLong
     * 
     */
    class StaticModelSphereLatLong: public ResourceConstructor<StaticModel, StaticModelSphereLatLong> {
    public:
        StaticModelSphereLatLong():
        ResourceConstructor<StaticModel, StaticModelSphereLatLong>{0}
        {}
        inline static std::string getResourceConstructorName() { return "sphereLatLong"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerResourceDB
     * @brief Constructs a rectangle mesh using the latitude-longitude method.
     * 
     * 
     * @see StaticMeshRectangleDimensions
     * 
     */
    class StaticModelRectangleDimensions: public ResourceConstructor<StaticModel, StaticModelRectangleDimensions> {
    public:
        StaticModelRectangleDimensions():
        ResourceConstructor<StaticModel, StaticModelRectangleDimensions>{0}
        {}
        inline static std::string getResourceConstructorName() { return "rectangleDimensions"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief Constructs a cuboid mesh based on its dimensions
     * 
     * @see StaticMeshCuboidDimensions
     * 
     */
    class StaticModelCuboidDimensions: public ResourceConstructor<StaticModel, StaticModelCuboidDimensions> {
    public:
        StaticModelCuboidDimensions():
        ResourceConstructor<StaticModel, StaticModelCuboidDimensions>{0}
        {}
        inline static std::string getResourceConstructorName() { return "cuboidDimensions"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

}
#endif
