#include <array>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "toymaker/engine/core/resource_database.hpp"
#include "toymaker/engine/vertex.hpp"
#include "toymaker/engine/model.hpp"
#include "toymaker/engine/material.hpp"
#include "toymaker/engine/spatial_query_basic_types.hpp"
#include "toymaker/engine/mesh.hpp"
#include "toymaker/engine/shapegen.hpp"

using namespace ToyMaker;

std::array<glm::vec2, 24> getCubemapTextureCoordinates(ColorBufferDefinition::CubemapLayout cubemapLayout);

std::shared_ptr<StaticMesh> generateSphereMesh(int nLatitude, int nMeridian, bool flipTextureY=false);
std::shared_ptr<StaticMesh> generateRectangleMesh(float width=2.f, float height=2.f, bool flipTextureY=false);
std::shared_ptr<StaticMesh> generateCuboidMesh(float width=2.f, float height=2.f, float depth=2.f, ColorBufferDefinition::CubemapLayout cubemapLayout=ColorBufferDefinition::CubemapLayout::ROW, bool flipTextureY=false);

std::shared_ptr<IResource> StaticMeshSphereLatLong::createResource(const nlohmann::json& methodParameters) {
    return generateSphereMesh(
        methodParameters.at("nLatitudes").get<uint32_t>(),
        methodParameters.at("nMeridians").get<uint32_t>(),
        methodParameters.find("flip_texture_y") != methodParameters.end()? methodParameters.at("flip_texture_y").get<bool>(): false
    );
}

std::shared_ptr<IResource> StaticMeshRectangleDimensions::createResource(const nlohmann::json& methodParameters) {
    return generateRectangleMesh(
        methodParameters.at("width").get<float>(),
        methodParameters.at("height").get<float>(),
        methodParameters.find("flip_texture_y") != methodParameters.end()? methodParameters.at("flip_texture_y").get<bool>(): false
    );
}

std::shared_ptr<IResource> StaticMeshCuboidDimensions::createResource(const nlohmann::json& methodParameters) {
    return generateCuboidMesh(
        methodParameters.at("width").get<float>(),
        methodParameters.at("height").get<float>(),
        methodParameters.at("depth").get<float>(),
        methodParameters.find("layout") != methodParameters.end()?
            methodParameters.at("layout").get<ColorBufferDefinition::CubemapLayout>():
            ColorBufferDefinition::CubemapLayout::ROW,
        methodParameters.find("flip_texture_y") != methodParameters.end()?
            methodParameters.at("flip_texture_y").get<bool>():
            false
    );
}

std::shared_ptr<IResource> StaticModelSphereLatLong::createResource(const nlohmann::json& methodParameters) {
    std::shared_ptr<StaticMesh> sphereMesh {
        ResourceDatabase::ConstructAnonymousResource<StaticMesh>({
            {"type", StaticMesh::getResourceTypeName()},
            {"method", StaticMeshSphereLatLong::getResourceConstructorName()},
            {"parameters", methodParameters}
        })
    };

    std::shared_ptr<Material> sphereMaterial { 
        ResourceDatabase::ConstructAnonymousResource<Material>({
            {"type", Material::getResourceTypeName()},
            {"method", MaterialFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"properties", methodParameters.at("material_properties").get<nlohmann::json::array_t>()},
            }}
        })
    };

    std::shared_ptr<StaticModel> sphereModel { 
        std::make_shared<StaticModel>(
            std::vector<std::shared_ptr<StaticMesh>>{ sphereMesh },
            std::vector<std::shared_ptr<Material>>{ sphereMaterial }
        )
    };

    return sphereModel;
}

std::shared_ptr<IResource> StaticModelRectangleDimensions::createResource(const nlohmann::json& methodParameters) {
    std::shared_ptr<StaticMesh> rectangleMesh {
        ResourceDatabase::ConstructAnonymousResource<StaticMesh>({
            {"type", StaticMesh::getResourceTypeName()},
            {"method", StaticMeshRectangleDimensions::getResourceConstructorName()},
            {"parameters", methodParameters}
        })
    };
    std::shared_ptr<Material> rectangleMaterial { 
        ResourceDatabase::ConstructAnonymousResource<Material>({
            {"type", Material::getResourceTypeName()},
            {"method", MaterialFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"properties", methodParameters.at("material_properties").get<nlohmann::json::array_t>()},
            }}
        })
    };

    std::shared_ptr<StaticModel> rectangleModel{
        std::make_shared<StaticModel>(
            std::vector<std::shared_ptr<StaticMesh>>{rectangleMesh},
            std::vector<std::shared_ptr<Material>>{rectangleMaterial}
        )
    };

    return rectangleModel;
}

std::shared_ptr<IResource> StaticModelCuboidDimensions::createResource(const nlohmann::json& methodParameters) {
    std::shared_ptr<StaticMesh> cuboidMesh {
        ResourceDatabase::ConstructAnonymousResource<StaticMesh>({
            {"type", StaticMesh::getResourceTypeName()},
            {"method", StaticMeshCuboidDimensions::getResourceConstructorName()},
            {"parameters", methodParameters}
        })
    };

    std::shared_ptr<Material> cuboidMaterial {
        ResourceDatabase::ConstructAnonymousResource<Material>({
            {"type", Material::getResourceTypeName()},
            {"method", MaterialFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"properties", methodParameters.at("material_properties").get<nlohmann::json::array_t>()},
            }}
        })
    };

    std::shared_ptr<StaticModel> cuboidModel {
        std::make_shared<StaticModel>(
            std::vector<std::shared_ptr<StaticMesh>>{cuboidMesh},
            std::vector<std::shared_ptr<Material>>{cuboidMaterial}
        )
    };
    return cuboidModel;
}

std::shared_ptr<StaticMesh> generateSphereMesh(int nLatitude, int nMeridian, bool flipTextureY)  {
    assert(nLatitude >= 1);
    assert(nMeridian >= 2);

    const glm::mat3 textureCoordinateTransform { flipTextureY ?
        glm::mat3 { // column major order
            {1.f, 0.f, 0.f},
            {0.f, -1.f, 0.f},
            {0.f, 1.f, 1.f},
        }:
        glm::mat3 { 1.f }
    };

    const int nVerticesPerLatitude { 2 * nMeridian };
    const int nVerticesTotal { 2 + nLatitude * nVerticesPerLatitude };

    const float angleVerticalDelta { 180.f / (1 + nLatitude) };
    const float angleHorizontalDelta{ 180.f / nMeridian };

    std::vector<BuiltinVertexData> vertices(nVerticesTotal);
    int currentIndex { 0 };
    for(int i{0}; i < 2 + nLatitude; ++i) {
        const float angleVertical { i * angleVerticalDelta };
        const int nPointsCurrentLatitude {
            i % (1 + nLatitude)? nVerticesPerLatitude: 1
        };
        for(int j{0}; j < nPointsCurrentLatitude; ++j) {
            const float angleHorizontal { j * angleHorizontalDelta };
            vertices[currentIndex].mPosition = glm::vec4(
                glm::sin(glm::radians(angleVertical)) * glm::sin(glm::radians(angleHorizontal)),
                glm::cos(glm::radians(angleVertical)),
                glm::sin(glm::radians(angleVertical)) * glm::cos(glm::radians(angleHorizontal)),
                1.f
            );
            vertices[currentIndex].mNormal = glm::vec4(glm::vec3(vertices[currentIndex].mPosition), 0.f);
            vertices[currentIndex].mTangent = glm::vec4(
                glm::sin(glm::radians(angleHorizontal+90.f)),
                0.f,
                glm::cos(glm::radians(angleHorizontal+90.f)),
                0.f
            );
            vertices[currentIndex].mUV3
                = vertices[currentIndex].mUV2
                = vertices[currentIndex].mUV1
                = static_cast<glm::vec2>(
                    textureCoordinateTransform 
                    * glm::vec3(
                        static_cast<float>(j)/nPointsCurrentLatitude, angleVertical / 180.f, 1.f
                    )
                );
            vertices[currentIndex].mColor = glm::vec4(1.f);
            ++currentIndex;
        }
    }

    //Generate elements, each set of 3 representing a triangle
    const int nTriangles {
        2*nVerticesPerLatitude * nLatitude
    };
    std::vector<GLuint> elements(3* static_cast<unsigned int>(nTriangles));
    int elementIndex {0};
    int previousBaseIndex {0};
    for(int i{1}; i < 2 + nLatitude; ++i) {
        const int nPointsCurrentLatitude {
            i%(1+nLatitude)? nVerticesPerLatitude: 1
        };
        const int nPointsPreviousLatitude {
            (i-1)%(1+nLatitude)? nVerticesPerLatitude: 1
        };
        const int currentBaseIndex {
            previousBaseIndex + nPointsPreviousLatitude
        };
        const int nJoiningFaces {std::max(nPointsCurrentLatitude, nPointsPreviousLatitude)};

        for(int j{0}; j < nJoiningFaces; ++j) {
            const int topleft { previousBaseIndex + (j % nPointsPreviousLatitude)};
            const int topright { previousBaseIndex + ((1+j) % nPointsPreviousLatitude)};
            const int bottomleft { currentBaseIndex + (j % nPointsCurrentLatitude) };
            const int bottomright  { currentBaseIndex + ((1+j) % nPointsCurrentLatitude)};

            if(bottomleft != bottomright) {
                elements[elementIndex++] = topleft;
                elements[elementIndex++] = bottomleft;
                elements[elementIndex++] = bottomright;
            }
            if(topleft != topright) {
                elements[elementIndex++] = topleft;
                elements[elementIndex++] = bottomright;
                elements[elementIndex++] = topright;
            }
        }
        previousBaseIndex = currentBaseIndex;
    }

    return std::make_shared<StaticMesh>(vertices, elements);
}

std::shared_ptr<StaticMesh> generateRectangleMesh(float width, float height, bool flipTextureY) {
    assert(width > 0.f);
    assert(height > 0.f);
    const glm::mat3 textureCoordinateTransform { flipTextureY ?
        glm::mat3 { // column major order
            {1.f, 0.f, 0.f},
            {0.f, -1.f, 0.f},
            {0.f, 1.f, 1.f},
        }:
        glm::mat3 { 1.f }
    };

    std::vector<BuiltinVertexData> vertices {
        {
            .mPosition {-width/2.f, height/2.f, 0.f, 1.f},
            .mNormal{0.f, 0.f, 1.f, 0.f},
            .mTangent{1.f, 0.f, 0.f, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {0.f, 1.f, 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {0.f, 1.f, 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {0.f, 1.f, 1.f}) },
        },
        {
            .mPosition {width/2.f, height/2.f, 0.f, 1.f},
            .mNormal{0.f, 0.f, 1.f, 0.f},
            .mTangent{1.f, 0.f, 0.f, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {1.f, 1.f, 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {1.f, 1.f, 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {1.f, 1.f, 1.f}) },
        },
        {
            .mPosition {width/2.f, -height/2.f, 0.f, 1.f},
            .mNormal {0.f, 0.f, 1.f, 0.f},
            .mTangent {1.f, 0.f, 0.f, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {1.f, 0.f, 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {1.f, 0.f, 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {1.f, 0.f, 1.f}) },
        },
        {
            .mPosition {-width/2.f, -height/2.f, 0.f, 1.f},
            .mNormal {0.f, 0.f, 1.f, 0.f},
            .mTangent {1.f, 0.f, 0.f, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {0.f, 0.f, 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {0.f, 0.f, 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {0.f, 0.f, 1.f}) },
        },
    };

    std::vector<GLuint> elements {
        {0}, {2}, {1},
        {0}, {3}, {2}
    };

    return std::make_shared<StaticMesh>(vertices, elements);
}

std::shared_ptr<StaticMesh> generateCuboidMesh(float width, float height, float depth, ColorBufferDefinition::CubemapLayout layout, bool flipTextureY) {
    assert(width > 0.f);
    assert(height > 0.f);
    assert(depth > 0.f);

    const glm::mat3 textureCoordinateTransform { flipTextureY ?
        glm::mat3 { // column major order
            {1.f, 0.f, 0.f},
            {0.f, -1.f, 0.f},
            {0.f, 1.f, 1.f},
        }:
        glm::mat3 { 1.f } // identity matrix
    };

    const std::array<glm::vec3, 8> boxCorners { VolumeBox{.mDimensions{width, height, depth}}.getVolumeRelativeBoxCorners() };
    const std::array<glm::vec2, 24> textureCoordinates{ getCubemapTextureCoordinates(layout) };

    // by default, assume that the cubemap texture is layed out in a row
    std::vector<BuiltinVertexData> vertices {
        // left bottom back
        {
            .mPosition {boxCorners[0], 1.f},
            .mNormal{glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mTangent{glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[0], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[0], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[0], 1.f}) },
        },
        {
            .mPosition {boxCorners[0], 1.f},
            .mNormal{glm::vec3{0.f, -1.f, 0.f}, 0.f},
            .mTangent{glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[1], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[1], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[1], 1.f}) },
        },
        {
            .mPosition {boxCorners[0], 1.f},
            .mNormal{glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mTangent{glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[2], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[2], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[2], 1.f}) },
        },

        // right bottom back
        {
            .mPosition {boxCorners[1], 1.f},
            .mNormal{glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mTangent{glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[3], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[3], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[3], 1.f}) },
        },
        {
            .mPosition {boxCorners[1], 1.f},
            .mNormal{glm::vec3{0.f, -1.f, 0.f}, 0.f},
            .mTangent{glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[4], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[4], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[4], 1.f}) },
        },
        {
            .mPosition {boxCorners[1], 1.f},
            .mNormal{glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mTangent{glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor{1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[5], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[5], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[5], 1.f}) },
        },

        // left top back
        {
            .mPosition {boxCorners[2], 1.f},
            .mNormal {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mTangent {glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[6], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[6], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[6], 1.f}) },
        },
        {
            .mPosition {boxCorners[2], 1.f},
            .mNormal {glm::vec3{0.f, 1.f, 0.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[7], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[7], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[7], 1.f}) },
        },
        {
            .mPosition {boxCorners[2], 1.f},
            .mNormal {glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[8], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[8], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3{textureCoordinates[8], 1.f}) },
        },

        // right top back
        {
            .mPosition {boxCorners[3], 1.f},
            .mNormal {glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mTangent {glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[9], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[9], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[9], 1.f}) },
        },
        {
            .mPosition {boxCorners[3], 1.f},
            .mNormal {glm::vec3{0.f, 1.f, 0.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[10], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[10], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[10], 1.f}) },
        },
        {
            .mPosition {boxCorners[3], 1.f},
            .mNormal {glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[11], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[11], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[11], 1.f}) },
        },

        // left bottom front
        {
            .mPosition {boxCorners[4], 1.f},
            .mNormal {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mTangent {glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[12], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[12], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[12], 1.f}) },
        },
        {
            .mPosition {boxCorners[4], 1.f},
            .mNormal {glm::vec3{0.f, -1.f, 0.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[13], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[13], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[13], 1.f}) },
        },
        {
            .mPosition {boxCorners[4], 1.f},
            .mNormal {glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mTangent {glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[14], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[14], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[14], 1.f}) },
        },

        // right bottom front
        {
            .mPosition {boxCorners[5], 1.f},
            .mNormal {glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mTangent {glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[15], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[15], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[15], 1.f}) },
        },
        {
            .mPosition {boxCorners[5], 1.f},
            .mNormal {glm::vec3{0.f, -1.f, 0.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[16], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[16], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[16], 1.f}) },
        },
        {
            .mPosition {boxCorners[5], 1.f},
            .mNormal {glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mTangent {glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[17], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[17], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[17], 1.f}) },
        },

        // left top front
        {
            .mPosition {boxCorners[6], 1.f},
            .mNormal {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mTangent {glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[18], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[18], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[18], 1.f}) },
        },
        {
            .mPosition {boxCorners[6], 1.f},
            .mNormal {glm::vec3{0.f, 1.f, 0.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[19], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[19], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[19], 1.f}) },
        },
        {
            .mPosition {boxCorners[6], 1.f},
            .mNormal {glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mTangent {glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[20], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[20], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[20], 1.f}) },
        },

        // right top front
        {
            .mPosition {boxCorners[7], 1.f},
            .mNormal {glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mTangent {glm::vec3{0.f, 0.f, -1.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[21], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[21], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[21], 1.f}) },
        },
        {
            .mPosition {boxCorners[7], 1.f},
            .mNormal {glm::vec3{0.f, 1.f, 0.f}, 0.f},
            .mTangent {glm::vec3{-1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[22], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[22], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[22], 1.f}) },
        },
        {
            .mPosition {boxCorners[7], 1.f},
            .mNormal {glm::vec3{0.f, 0.f, 1.f}, 0.f},
            .mTangent {glm::vec3{1.f, 0.f, 0.f}, 0.f},
            .mColor {1.f, 1.f, 1.f, 1.f},
            .mUV1{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[23], 1.f}) },
            .mUV2{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[23], 1.f}) },
            .mUV3{ static_cast<glm::vec2>(textureCoordinateTransform * glm::vec3 {textureCoordinates[23], 1.f}) },
        },
    };


    std::vector<GLuint> elements {
        // left face triangles
        0*3+0, 4*3+0, 6*3+0,
        0*3+0, 6*3+0, 2*3+0,

        // right face triangles
        5*3+0, 1*3+0, 3*3+0,
        5*3+0, 3*3+0, 7*3+0,

        // bottom face triangles
        5*3+1, 4*3+1, 0*3+1,
        5*3+1, 0*3+1, 1*3+1,

        // top face triangles
        2*3+1, 3*3+1, 7*3+1,
        2*3+1, 7*3+1, 6*3+1,

        // back face triangles
        1*3+2, 0*3+2, 2*3+2,
        1*3+2, 2*3+2, 3*3+2,

        // front face triangles
        4*3+2, 5*3+2, 7*3+2,
        4*3+2, 7*3+2, 6*3+2,
    };

    return std::make_shared<StaticMesh>(vertices, elements);   
}

std::array<glm::vec2, 24> getCubemapTextureCoordinates(ColorBufferDefinition::CubemapLayout cubemapLayout) {

    // NOTE:  Each corner of a cuboid is shared by 3 faces.  The texture contains an unwrapped version
    // of the cuboid layed out in the manner specified by cubemap layout.  For each face, the same corner
    // will use different texture coordinates
    std::array<glm::vec2, 24> textureCoordinates;

    switch(cubemapLayout) {
        case ColorBufferDefinition::CubemapLayout::NA:
        case ColorBufferDefinition::CubemapLayout::ROW:
            textureCoordinates = {
                // left bottom back
                glm::vec2{ 2.f / 6.f, 0.f }, // left face
                glm::vec2{ 3.f / 6.f, 1.f }, // bottom face
                glm::vec2{ 5.f / 6.f, 0.f }, // back face

                // right bottom back
                glm::vec2{ 0.f, 0.f }, // right face
                glm::vec2{ 4.f / 6.f, 1.f}, // bottom face
                glm::vec2{ 6.f / 6.f, 0.f }, // back face

                // left top back
                glm::vec2{ 2.f / 6.f, 1.f }, // left face
                glm::vec2{ 2.f / 6.f, 0.f }, // top face
                glm::vec2{ 5.f / 6.f, 1.f }, // back face

                // right top back
                glm::vec2{ 0.f, 1.f }, // right face
                glm::vec2{ 3.f / 6.f, 0.f }, // top face
                glm::vec2{ 6.f / 6.f, 1.f }, // back face

                // left bottom front
                glm::vec2{ 1.f / 6.f, 0.f }, // left face
                glm::vec2{ 3.f / 6.f, 0.f }, // bottom face
                glm::vec2{ 5.f / 6.f, 0.f }, // front face

                // right bottom front
                glm::vec2{ 1.f / 6.f, 0.f }, // right face
                glm::vec2{ 4.f / 6.f, 0.f }, // bottom face
                glm::vec2{ 4.f / 6.f, 0.f }, // front face

                // left top front
                glm::vec2{ 1.f / 6.f, 1.f }, // left face
                glm::vec2{ 2.f / 6.f, 1.f }, // top face
                glm::vec2{ 5.f / 6.f, 1.f }, // front face

                // right top front
                glm::vec2{ 1.f / 6.f, 1.f }, // right face
                glm::vec2{ 3.f / 6.f, 1.f }, // top face
                glm::vec2{ 4.f / 6.f, 1.f }, // front face
            };
        break;
    };

    return textureCoordinates;
}
