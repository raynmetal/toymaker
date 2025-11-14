#include <iostream> 
#include <vector>
#include <utility>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "toymaker/engine/window_context_manager.hpp"
#include "toymaker/engine/material.hpp"
#include "toymaker/engine/texture.hpp"
#include "toymaker/engine/shader_program.hpp"
#include "toymaker/engine/mesh.hpp"
#include "toymaker/engine/vertex.hpp"
#include "toymaker/engine/model.hpp"

using namespace ToyMaker;

void processAssimpNode(aiNode* node, const aiScene* pAiScene, std::vector<std::shared_ptr<StaticMesh>>& meshHandles, std::vector<std::shared_ptr<Material>>& materialHandles);
std::vector<std::shared_ptr<Texture>> loadAssimpTextures(aiMaterial* pAiMaterial, aiTextureType textureType);
void processAssimpMesh(aiMesh* pAiMesh, const aiScene* pAiScene, std::vector<std::shared_ptr<StaticMesh>>& meshHandles, std::vector<std::shared_ptr<Material>>& materialHandles);
std::shared_ptr<StaticMesh> buildMesh(aiMesh* pAiMesh);

StaticModel::StaticModel(const std::vector<std::shared_ptr<StaticMesh>>& meshHandles, const std::vector<std::shared_ptr<Material>>& materialHandles):
Resource<StaticModel>{0},
mMeshHandles { meshHandles },
mMaterialHandles { materialHandles }
{
    assert(meshHandles.size() > 0 && meshHandles.size() == materialHandles.size()
        && "Every mesh in the mesh list must have its corresponding material in the material list"
    );
}
StaticModel::~StaticModel() {
    free();
}
StaticModel::StaticModel(StaticModel&& other):
Resource<StaticModel>{0}
{
    stealResources(other);
}
StaticModel& StaticModel::operator=(StaticModel&& other) {
    if(this == &other) 
        return *this;

    free();
    stealResources(other);

    return *this;
}
StaticModel::StaticModel(const StaticModel& other):
Resource<StaticModel>{0}
{
    copyResources(other);
}
StaticModel& StaticModel::operator=(const StaticModel& other) {
    if(this == &other)
        return *this;

    free();
    copyResources(other);

    return *this;
}
std::vector<std::shared_ptr<StaticMesh>> StaticModel::getMeshHandles() const { return mMeshHandles; }
std::vector<std::shared_ptr<Material>> StaticModel::getMaterialHandles() const { return mMaterialHandles; }
void StaticModel::free() {
    mMeshHandles.clear();
    mMaterialHandles.clear();
}
void StaticModel::stealResources(StaticModel& other) {
    std::swap(mMeshHandles, other.mMeshHandles);
    std::swap(mMaterialHandles, other.mMaterialHandles);

    // Prevent other from destroying our resources when its
    // destructor is called
    other.releaseResource();
}
void StaticModel::copyResources(const StaticModel& other) {
    mMeshHandles = other.mMeshHandles;
    mMaterialHandles = other.mMaterialHandles;
}
void StaticModel::destroyResource() {
    free();
}
void StaticModel::releaseResource() {}

StaticModelFromFile::StaticModelFromFile():
ResourceConstructor<StaticModel, StaticModelFromFile>{0}
{}

std::shared_ptr<IResource> StaticModelFromFile::createResource(const nlohmann::json& methodParameters) {

    std::string modelPath { methodParameters.at("path").get<std::string>() };
    Assimp::Importer* pImporter { WindowContext::getInstance().getAssetImporter() };
    const aiScene* pAiScene {
        pImporter->ReadFile(
            modelPath,
            aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace
        )
    };

    if(
        !pAiScene 
        || (pAiScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
        || !(pAiScene->mRootNode)
    ) {
        std::cout << "ERROR::ASSIMP:: " << pImporter->GetErrorString() << std::endl;
    }
    assert(pAiScene && !(pAiScene->mFlags&AI_SCENE_FLAGS_INCOMPLETE) && pAiScene->mRootNode
        && "Could not read model file"
    );

    std::vector<std::shared_ptr<StaticMesh>> meshHandles {};
    std::vector<std::shared_ptr<Material>> materialHandles{};
    processAssimpNode(pAiScene->mRootNode, pAiScene, meshHandles, materialHandles);
    assert(meshHandles.size() > 0 && meshHandles.size() == materialHandles.size() &&
        "There must be as many materials as there are meshes"
    );

    if(methodParameters.find("material_overrides") != methodParameters.end()) {
        for(
            auto override { methodParameters.at("material_overrides").begin() };
            override != methodParameters.at("material_overrides").end();
            ++override
        ) {
            assert(std::all_of(override.key().begin(), override.key().end(), ::isdigit)
                && "Invalid material key; key must be a string comprised of numeric characters"
            );
            std::size_t materialIndex { std::stoull(override.key()) };
            assert(materialIndex < materialHandles.size());
            Material::ApplyOverrides(override.value(), materialHandles[materialIndex]);
        }
    }

    return std::make_shared<StaticModel>(meshHandles, materialHandles);
}

void processAssimpNode(aiNode* pAiNode, const aiScene* pAiScene, std::vector<std::shared_ptr<StaticMesh>>& meshHandles, std::vector<std::shared_ptr<Material>>& materialHandles) {
    for(std::size_t  i{0}; i < pAiNode->mNumMeshes; ++i) {
        aiMesh* pAiMesh {
            pAiScene->mMeshes[pAiNode->mMeshes[i]]
        };
        processAssimpMesh(pAiMesh, pAiScene, meshHandles, materialHandles);
    }

    for(std::size_t i{0}; i < pAiNode->mNumChildren; ++i) {
        processAssimpNode(pAiNode->mChildren[i], pAiScene, meshHandles, materialHandles);
    }
}

std::shared_ptr<StaticMesh> buildMesh(aiMesh* pAiMesh) {
    std::vector<BuiltinVertexData> vertices {};
    std::vector<GLuint> elements {};
    nlohmann::json meshDescription {
        {"vertices", nlohmann::json::array()},
        {"elements", nlohmann::json::array()},
    };
    for (std::size_t i{0}; i < pAiMesh->mNumVertices; ++i) {
        // TODO: make fetching vertex data more robust. There is no
        // guarantee that a mesh will contain vertex colours or
        // texture sampling coordinates
        vertices.push_back({
            .mPosition { // position
                pAiMesh->mVertices[i].x,
                pAiMesh->mVertices[i].y,
                pAiMesh->mVertices[i].z,
                1.f
            },
            .mNormal { // normal
                pAiMesh->mNormals[i].x,
                pAiMesh->mNormals[i].y,
                pAiMesh->mNormals[i].z,
                0.f
            },
            .mTangent { // tangent
                pAiMesh->mTangents[i].x,
                pAiMesh->mTangents[i].y,
                pAiMesh->mTangents[i].z,
                0.f
            },
            .mColor { 1.f, 1.f, 1.f, 1.f },
            .mUV1 {
                pAiMesh->mTextureCoords[0][i].x,
                pAiMesh->mTextureCoords[0][i].y
            },
            .mUV2{},
            .mUV3{},
        });
    }

    for(std::size_t i{0}; i < pAiMesh->mNumFaces; ++i) {
        aiFace face = pAiMesh->mFaces[i];
        for(std::size_t elementIndex{0}; elementIndex < face.mNumIndices; ++elementIndex) {
            elements.push_back(face.mIndices[elementIndex]);
        }
    }
    return std::make_shared<StaticMesh>(vertices, elements);
}

void processAssimpMesh(aiMesh* pAiMesh, const aiScene* pAiScene, std::vector<std::shared_ptr<StaticMesh>>& meshHandles, std::vector<std::shared_ptr<Material>>& materialHandles) {

    std::vector<std::shared_ptr<Texture>> textureHandles {};

    meshHandles.push_back(buildMesh(pAiMesh));
    materialHandles.push_back(
        std::make_shared<Material>()
    );

    // Load textures
    aiMaterial* pAiMaterial {
        pAiScene->mMaterials[pAiMesh->mMaterialIndex]
    };

    // TODO: Make associating textures with their respective material
    // props more elegant somehow?
    std::vector<std::shared_ptr<Texture>> textureHandlesAlbedo {
        loadAssimpTextures(pAiMaterial, aiTextureType_DIFFUSE)
    };
    std::vector<std::shared_ptr<Texture>> textureHandlesNormal {
        loadAssimpTextures(pAiMaterial, aiTextureType_NORMALS)
    };
    std::vector<std::shared_ptr<Texture>> textureHandlesSpecular {
        loadAssimpTextures(pAiMaterial, aiTextureType_SPECULAR)
    };

    if(!textureHandlesAlbedo.empty()) {
        materialHandles.back()->updateTextureProperty(
            "textureAlbedo", textureHandlesAlbedo.back()
        );
        materialHandles.back()->updateIntProperty(
            "usesTextureAlbedo", true
        );
    }
    if(!textureHandlesNormal.empty()) {
        materialHandles.back()->updateTextureProperty(
            "textureNormal", textureHandlesNormal.back()
        );
        materialHandles.back()->updateIntProperty(
            "usesTextureNormal", true
        );
    }
    if(!textureHandlesSpecular.empty()) {
        materialHandles.back()->updateTextureProperty(
            "textureSpecular", textureHandlesSpecular.back()
        );
        materialHandles.back()->updateIntProperty(
            "usesTextureSpecular", true
        );
    }
}

std::vector<std::shared_ptr<Texture>> loadAssimpTextures(aiMaterial* pAiMaterial, aiTextureType textureType) {
    // build up a list of texture pointers, adding textures
    // to this model if it isn't already present
    std::vector<std::shared_ptr<Texture>> textureHandles {};
    std::size_t textureCount {
        pAiMaterial->GetTextureCount(
            textureType
        )
    };

    for(std::size_t i{0}; i < textureCount; ++i) {
        aiString aiTextureName;
        pAiMaterial->GetTexture(textureType, i, &aiTextureName);
        std::string textureName { aiTextureName.C_Str() };

        if(!ResourceDatabase::HasResourceDescription(textureName)){
            nlohmann::json textureDescription {
                {"name", textureName},
                {"type", Texture::getResourceTypeName()},
                {"method", TextureFromFile::getResourceConstructorName()},
                {"parameters", {
                    {"path", textureName}
                }}
            };
            ResourceDatabase::AddResourceDescription(textureDescription);
        }
        
        textureHandles.push_back(
            ResourceDatabase::GetRegisteredResource<Texture>(textureName)
        );
    }

    return textureHandles;
}
