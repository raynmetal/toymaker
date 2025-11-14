#include <iostream>
#include <GL/glew.h>
#include "toymaker/engine/instance.hpp"

using namespace ToyMaker;

BaseInstanceAllocator::~BaseInstanceAllocator() {
    if(!mUploaded) return;
    glDeleteBuffers(1, &mVertexBufferIndex);
};

InstanceLayout BaseInstanceAllocator::getInstanceLayout() const {
    return mInstanceLayout;
}

void BaseInstanceAllocator::_upload() {
    if(mUploaded) return;

    upload();
    assert(mVertexBufferIndex);
    mUploaded = true;
}

void BaseInstanceAllocator::unload() {
    if(!mUploaded) return;

    glDeleteBuffers(1, &mVertexBufferIndex);
    mVertexBufferIndex = 0;

    mUploaded = false;
}

void BaseInstanceAllocator::setAttributePointers(const InstanceLayout& shaderInstanceLayout, std::size_t startingOffset) {
    assert(mUploaded);
    assert(shaderInstanceLayout.isSubsetOf(mInstanceLayout));

    const std::size_t stride {mInstanceLayout.computeStride()};
    const std::vector<InstanceAttributeDescriptor>& attributeDescList { mInstanceLayout.getAttributeList() };
    const std::vector<InstanceAttributeDescriptor>& shaderAttributeDescList { shaderInstanceLayout.getAttributeList() };
    GLint programID { 0 };
    glGetIntegerv(GL_CURRENT_PROGRAM, &programID);
    std::size_t currentOffset {0};
    std::size_t shaderInstanceAttributeIndex {0};

    for(
        std::size_t i{0};
        (
            i < attributeDescList.size() 
            && shaderInstanceAttributeIndex < shaderAttributeDescList.size()
        );
        ++i
    ) {
        const InstanceAttributeDescriptor& attributeDesc = attributeDescList[i];
        const InstanceAttributeDescriptor& shaderAttributeDesc = shaderAttributeDescList[shaderInstanceAttributeIndex];

        if(attributeDesc == shaderAttributeDesc) {
            GLint layoutLocation {attributeDesc.mLayoutLocation};
            if(layoutLocation == DefaultInstanceAttributeLocations::RUNTIME) {
                layoutLocation = glGetAttribLocation(programID, attributeDesc.mName.c_str());
            }
            switch(attributeDesc.mType){
                case GL_INT:
                case GL_UNSIGNED_INT:
                    glVertexAttribIPointer(
                        layoutLocation,
                        attributeDesc.mNComponents,
                        attributeDesc.mType,
                        stride,
                        reinterpret_cast<void*>(startingOffset + currentOffset)
                    );
                break;
                case GL_FLOAT:
                default:
                    glVertexAttribPointer(
                        layoutLocation,
                        attributeDesc.mNComponents,
                        attributeDesc.mType,
                        GL_FALSE,
                        stride,
                        reinterpret_cast<void*>(startingOffset + currentOffset)
                    );
                break;
            }
            glEnableVertexAttribArray(layoutLocation);
            glVertexAttribDivisor(layoutLocation, 1);
            ++shaderInstanceAttributeIndex;
        }
        GLenum error = glGetError();
        if(error!= GL_FALSE) {
            glewGetErrorString(error);
            std::cout << "Error occurred during instance attribute setting: "  << error
                << ":" << glewGetErrorString(error) << std::endl;
            throw error;
        }
        currentOffset += attributeDesc.mSize;
    }

    assert(shaderInstanceAttributeIndex == shaderAttributeDescList.size());
}

void BaseInstanceAllocator::bind(const InstanceLayout& shaderInstanceLayout) {
    if(!mUploaded) {
        _upload();
    }
    assert(mUploaded);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferIndex);
        setAttributePointers(shaderInstanceLayout);
        GLenum error = glGetError();
        if(error!= GL_FALSE) {
            glewGetErrorString(error);
            std::cout << "Error occurred during instance attribute setting: "  << error
                << ":" << glewGetErrorString(error) << std::endl;
            throw error;
        }
}

void BaseInstanceAllocator::unbind() {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void BuiltinModelMatrixAllocator::upload() {
    if(isUploaded()) return;

    glGenBuffers(1, &mVertexBufferIndex);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferIndex);
        glBufferData(
            GL_ARRAY_BUFFER,
            mModelMatrices.size() * sizeof(glm::mat4),
            mModelMatrices.data(),
            GL_DYNAMIC_DRAW
        );
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
