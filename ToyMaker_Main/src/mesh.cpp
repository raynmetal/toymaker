#include <vector>
#include <map>
#include <iostream>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include <glm/glm.hpp>

#include "toymaker/engine/vertex.hpp"
#include "toymaker/engine/mesh.hpp"

using namespace ToyMaker;

StaticMesh::StaticMesh(
    const std::vector<BuiltinVertexData>& vertices,
    const std::vector<GLuint>& elements,
    GLuint vertexBuffer,
    GLuint elementBuffer,
    bool isUploaded
) :
Resource<StaticMesh> {0},
mVertices { vertices },
mElements { elements },
mVertexLayout{ BuiltinVertexLayout },
mIsUploaded { isUploaded },
mVertexBuffer { vertexBuffer },
mElementBuffer  { elementBuffer }
{}

StaticMesh::~StaticMesh() {
    destroyResource();
}

StaticMesh::StaticMesh(StaticMesh&& other):
Resource<StaticMesh> {0},
mVertices { other.mVertices },
mElements { other.mElements },
mVertexLayout { BuiltinVertexLayout },
mIsUploaded { other.mIsUploaded },
mVertexBuffer{ other.mVertexBuffer },
mElementBuffer{ other.mElementBuffer }
{
    // Prevent other from removing our resources when its destructor is called
    other.releaseResource();
}

StaticMesh::StaticMesh(const StaticMesh& other):
Resource<StaticMesh> {0},
mVertices { other.mVertices },
mElements { other.mElements },
mVertexLayout { BuiltinVertexLayout },
mIsUploaded { false },
mVertexBuffer { 0 },
mElementBuffer { 0 }
{}

StaticMesh& StaticMesh::operator=(StaticMesh&& other) {
    if(&other == this) 
        return *this;

    destroyResource();

    mVertices = other.mVertices;
    mElements = other.mElements;
    mIsUploaded = other.mIsUploaded;
    mVertexBuffer = other.mVertexBuffer;
    mElementBuffer = other.mElementBuffer;

    other.releaseResource();

    return *this;
}

StaticMesh& StaticMesh::operator=(const StaticMesh& other) {
    if(&other == this) 
        return *this;
    
    destroyResource();

    mVertices = other.mVertices;
    mElements = other.mElements;
    mIsUploaded = false;
    mVertexBuffer = 0;
    mElementBuffer = 0;

    return *this;
}

void StaticMesh::bind(const VertexLayout& shaderVertexLayout) {
    if(!mIsUploaded) {
        upload();
    }
    assert(mIsUploaded);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffer);
        setAttributePointers(shaderVertexLayout);
        GLenum error = glGetError();
        if(error!= GL_FALSE) {
            glewGetErrorString(error);
            std::cout << "Error occurred during mesh attribute setting: "  << error
                << ":" << glewGetErrorString(error) << std::endl;
            throw error;
        }
}

void StaticMesh::upload() {
    if(mIsUploaded) return;
    //   TODO: somehow have the base class take care of more of the memory 
    // management? Regardless of the type of vertex data, so long as we have
    // a pointer to the underlying array, there's no reason we shouldn't
    // be able to upload data in the base class rather than leaving it to
    // the subclass
    //   Not sure how we can achieve this.

    // Generate names for our vertex, element, and array buffers
    glGenBuffers(1, &mVertexBuffer);
    glGenBuffers(1, &mElementBuffer);

    // Set up the buffer objects for this mesh
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(BuiltinVertexData), mVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, mElements.size() * sizeof(GLuint), mElements.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    assert(mVertexBuffer);
    assert(mElementBuffer);
    mIsUploaded = true;
}

void StaticMesh::setAttributePointers(const VertexLayout& shaderVertexLayout, std::size_t startingOffset) {
    assert(mIsUploaded);
    assert(shaderVertexLayout.isSubsetOf(mVertexLayout));

    const std::size_t stride { mVertexLayout.computeStride() };
    const std::vector<VertexAttributeDescriptor>& attributeDescList { mVertexLayout.getAttributeList() };
    const std::vector<VertexAttributeDescriptor>& shaderAttributeDescList { shaderVertexLayout.getAttributeList() };

    std::size_t currentOffset {0};
    std::size_t shaderVertexAttributeIndex {0};
    for(std::size_t i {0}; i < attributeDescList.size() && shaderVertexAttributeIndex < shaderAttributeDescList.size(); ++i) {
        const VertexAttributeDescriptor& attributeDesc = attributeDescList[i];
        const VertexAttributeDescriptor& shaderAttributeDesc = shaderAttributeDescList[shaderVertexAttributeIndex];

        if(attributeDesc == shaderAttributeDesc){
            switch(attributeDesc.mType){
                case GL_INT:
                case GL_UNSIGNED_INT:
                    glVertexAttribIPointer(
                        attributeDesc.mLayoutLocation,
                        attributeDesc.mNComponents,
                        attributeDesc.mType,
                        stride,
                        reinterpret_cast<void*>(startingOffset + currentOffset)
                    );
                break;
                case GL_FLOAT:
                default:
                    glVertexAttribPointer(
                        attributeDesc.mLayoutLocation,
                        attributeDesc.mNComponents,
                        attributeDesc.mType,
                        GL_FALSE,
                        stride,
                        reinterpret_cast<void*>(startingOffset + currentOffset)
                    );
                break;
            }
            glEnableVertexAttribArray(attributeDesc.mLayoutLocation);
            GLenum error { glGetError() };
            if(error!= GL_FALSE) {
                std::cout << "Error occurred during mesh attribute enabling: "  << error
                    << ":" << glewGetErrorString(error) << std::endl;
                throw error;
            }
            ++shaderVertexAttributeIndex;
        }

        currentOffset += attributeDesc.mSize;
    }

    assert(shaderVertexAttributeIndex == shaderAttributeDescList.size());
}

void StaticMesh::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void StaticMesh::unload() {
    if(!mIsUploaded) return;

    glDeleteBuffers(1, &mVertexBuffer);
    mVertexBuffer = 0;
    glDeleteBuffers(1, &mElementBuffer);
    mElementBuffer = 0;

    mIsUploaded = false;
}

void StaticMesh::destroyResource() {
    unload();
}

void StaticMesh::releaseResource() {
    if(!mIsUploaded) return;

    mVertexBuffer = 0;
    mElementBuffer = 0;

    mIsUploaded = false;
}

VertexLayout StaticMesh::getVertexLayout() const {
    return mVertexLayout;
}

std::shared_ptr<IResource> StaticMeshFromDescription::createResource(const nlohmann::json& methodParameters) {
    std::vector<BuiltinVertexData> vertices {};
    std::vector<GLuint> elements {};
    for(const nlohmann::json& vertexParameters: methodParameters.at("vertices").get<std::vector<nlohmann::json>>()) {
        vertices.push_back(vertexParameters);
    }
    for(GLuint element: methodParameters.at("elements").get<std::vector<GLuint>>()) {
        elements.push_back(element);
    }
    return std::make_shared<StaticMesh>(vertices, elements);
}

std::vector<BuiltinVertexData>::iterator StaticMesh::getVertexListBegin() {
    return mVertices.begin();
}
std::vector<BuiltinVertexData>::const_iterator StaticMesh::getVertexListBegin() const {
    return mVertices.cbegin();
}
std::vector<BuiltinVertexData>::iterator StaticMesh::getVertexListEnd() {
    return mVertices.end();
}
std::vector<BuiltinVertexData>::const_iterator StaticMesh::getVertexListEnd() const {
    return mVertices.cend();
}

// BuiltinMesh::BuiltinMesh(aiMesh* pAiMesh) 
// : BaseMesh{ BuiltinVertexLayout } 
// {
//     for (std::size_t i{0}; i < pAiMesh->mNumVertices; ++i) {
//         // TODO: make fetching vertex data more robust. There is no
//         // guarantee that a mesh will contain vertex colours or
//         // texture sampling coordinates
//         mVertices.push_back({
//             { // position
//                 pAiMesh->mVertices[i].x,
//                 pAiMesh->mVertices[i].y,
//                 pAiMesh->mVertices[i].z,
//                 1.f
//             },
//             { // normal
//                 pAiMesh->mNormals[i].x,
//                 pAiMesh->mNormals[i].y,
//                 pAiMesh->mNormals[i].z,
//                 0.f
//             },
//             { // tangent
//                 pAiMesh->mTangents[i].x,
//                 pAiMesh->mTangents[i].y,
//                 pAiMesh->mTangents[i].z,
//                 0.f
//             },
//             { 1.f, 1.f, 1.f, 1.f },
//             {
//                 pAiMesh->mTextureCoords[0][i].x,
//                 pAiMesh->mTextureCoords[0][i].y
//             }
//         });
//     }

//     for(std::size_t i{0}; i < pAiMesh->mNumFaces; ++i) {
//         aiFace face = pAiMesh->mFaces[i];
//         for(std::size_t elementIndex{0}; elementIndex < face.mNumIndices; ++elementIndex) {
//             mElements.push_back(face.mIndices[elementIndex]);
//         }
//     }
// }

