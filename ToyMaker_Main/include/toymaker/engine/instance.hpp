/**
 * @ingroup ToyMakerRenderSystem
 * @file instance.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief A wrapper over regular shader attributes intended to be used as "instance" attributes, i.e., ones that change after what would traditionally be one-or-more draw calls in the same render stage.
 * @version 0.3.2
 * @date 2025-09-02
 * 
 */

#ifndef FOOLSENGINE_INSTANCE
#define FOOLSENGINE_INSTANCE

#include <vector>
#include <string>

#include <glm/glm.hpp>
#include <GL/glew.h>

namespace ToyMaker {
    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Attribute locations, per existing shaders.
     * 
     */
    enum DefaultInstanceAttributeLocations : int {
        FIXED_MATRIXMODEL=7,
        RUNTIME=-8,
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A struct describing the name and type of each attribute designated as an instance attribute.
     * 
     */
    struct InstanceAttributeDescriptor {
        /**
         * @brief Constructs a new instance attribute descriptor.
         * 
         * @param name The name of the attribute.
         * @param layoutLocation The OpenGL attribute location of the attribute.
         * @param nComponents The number of dimensions this attribute has, a multiplier on the storage size of type.
         * @param type The actual OpenGL type of the attribute, like GL_FLOAT
         */
        InstanceAttributeDescriptor(const std::string& name, const GLint layoutLocation, GLuint nComponents, GLenum type=GL_FLOAT) 
        : mName {name}, mLayoutLocation { layoutLocation }, 
            mNComponents {nComponents}, mType {type}, 
            mSize{GetGLTypeSize(type) * nComponents}
        {}

        /**
         * @brief Name of the attribute.
         * 
         */
        const std::string mName;

        /**
         * @brief The OpenGL shader attribute location of this attribute.
         * 
         */
        const GLint mLayoutLocation;

        /**
         * @brief The number or components or dimensions that represent this attribute.
         * 
         */
        const GLuint mNComponents;

        /**
         * @brief The underlying OpenGL type of the attribute.
         * 
         */
        const GLenum mType;

        /**
         * @brief The size of the attribute, computed as size of mType * mNComponents.
         * 
         */
        const std::size_t mSize;

        /**
         * @brief Compares two vertex attributes for equality
         * 
         * @todo code duplicated from VertexAttributeDescriptor.  Refactor needed.
         * 
         * @param other The attribute descriptor this one is being compared to.
         * 
         * @retval true Attributes are the same;
         * 
         * @retval false Attributes are different;
         */
        bool operator==(const InstanceAttributeDescriptor& other) const {
            if(
                other.mName == mName 
                && other.mLayoutLocation == mLayoutLocation 
                && other.mNComponents == mNComponents
                && other.mType == mType
                && other.mSize == mSize
            ) return true;
            return false;   
        }

    private:
        /**
         * @brief Returns the size of a few known OpenGL component types, throws an error otherwise.
         * 
         * @param type The OpenGL type of the attribute component.
         * @return std::size_t The attribute component's size, in bytes.
         */
        static std::size_t GetGLTypeSize(GLenum type){
            std::size_t componentSize;
            switch(type) {
                case GL_FLOAT:
                    // fall through
                case GL_INT:
                    // fall through
                case GL_UNSIGNED_INT:
                    componentSize = sizeof(GLfloat);
                    break;

                default:
                    assert(false && "Unrecognized or unsupported type");
            }
            return componentSize;
        }
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Object representing the layout of one set of related attributes representing (presumably) one object or instance.
     * 
     * In implementation it is essentially a list of attribute descriptors.  The descriptors are then used to upload data to GPU memory, and also help bind (a subset of) attributes correctly to shaders for some rendering stage.
     * 
     */
    struct InstanceLayout {
    public:
        /**
         * @brief Constructs a new instance layout object
         * 
         * @param attributeList List of attribute descriptors.
         */
        InstanceLayout(const std::vector<InstanceAttributeDescriptor>& attributeList) : mAttributeList{attributeList} {}

        /**
         * @brief Returns this layout's attribute list.
         * 
         * @return std::vector<InstanceAttributeDescriptor> This layout's attribute descriptors.
         */
        std::vector<InstanceAttributeDescriptor> getAttributeList() const {
            return mAttributeList;
        }

        /**
         * @brief Computes the stride for this layout, i.e., the number of bytes separating the start of one instance's attributes and the start of the next one's.
         * 
         * @return std::size_t The size, in memory, of a single instance's attributes.
         */
        std::size_t computeStride() const {
            std::size_t stride {0};
            for(const auto& attribute: mAttributeList) {
                stride += attribute.mSize;
            }
            return stride;
        }

        /**
         * @brief Computes the offset of a specific attribute from the start of the instance, in bytes.
         * 
         * @param attributeIndex The index to the attribute descriptor whose offset we want.
         * @return std::size_t The number of bytes from the start of the layout to the start of the desired attribute.
         */
        std::size_t computeRelativeOffset(std::size_t attributeIndex) const {
            assert(attributeIndex < mAttributeList.size());
            std::size_t baseOffset {0};
            for(std::size_t i{0} ; i < attributeIndex; ++i) {
                baseOffset += mAttributeList[i].mSize;
            }
            return baseOffset;
        }

        /**
         * @brief Tests whether another InstanceLayout has the same attributes in the same order as this one, where some attributes may be absent from the other.
         * 
         * @param other The layout this one is being compared with.
         * @retval true The other layout is a subset of this one.
         * @retval false The other layout is not a subset of this one.
         */
        bool isSubsetOf(const InstanceLayout& other) const {
            if(mAttributeList.size() > other.mAttributeList.size()) return false;

            std::size_t myAttributeIndex {0};
            for(std::size_t i{0}; i < other.mAttributeList.size() && myAttributeIndex < mAttributeList.size(); ++i) {
                if(other.mAttributeList[i] == mAttributeList[myAttributeIndex]) {
                    ++ myAttributeIndex;
                    if(myAttributeIndex == mAttributeList.size()) return true;
                }
            }

            return false;
        }

    private:
        /**
         * @brief The list of attribute descriptors that make up this InstanceLayout.
         * 
         */
        std::vector<InstanceAttributeDescriptor> mAttributeList;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Class that is responsible for taking an instance layout and correctly uploading it to the GPU.
     * 
     * It is also responsible for binding attribute data as specified by a layout to some shader attribute.
     * 
     */
    class BaseInstanceAllocator {
    public:
        /**
         * @brief Construct a new base instance allocator object
         * 
         * @param instanceLayout The layout this instance allocator is responsible for.
         */
        BaseInstanceAllocator(const InstanceLayout& instanceLayout)
        : mInstanceLayout(instanceLayout) 
        {}

        BaseInstanceAllocator(BaseInstanceAllocator&& other) = delete;
        BaseInstanceAllocator(const BaseInstanceAllocator& other) = delete;

        BaseInstanceAllocator& operator=(BaseInstanceAllocator&& other) = delete;
        BaseInstanceAllocator& operator=(const BaseInstanceAllocator& other) = delete;

        /**
         * @brief Destroys the allocator object.
         * 
         */
        virtual ~BaseInstanceAllocator();

        /**
         * @brief Gets the instance attribute layout for this object.
         * 
         * @return InstanceLayout This object's instance layout.
         */
        InstanceLayout getInstanceLayout() const;

        /**
         * @brief Binds a (subset of) this object's instance attributes to the currently active shader.
         * 
         * @param shaderInstanceLayout The instance layout requested by the shader, which is expected to be a subset of this allocator's layout.
         */
        void bind(const InstanceLayout& shaderInstanceLayout);

        /**
         * @brief Unbinds this object's instance attributes.
         * 
         */
        void unbind();

        /**
         * @brief Tests whether the attribute data associated with this allocator has been uploaded to memory.
         * 
         * @retval true Attribute data has been uploaded.
         * @retval false Attribute data has not been uploaded.
         */
        bool isUploaded() { return mUploaded; }

    protected:
        /**
         * @brief Uploads this object's attribute data to GPU memory.
         * 
         */
        virtual void upload() = 0;

        /**
         * @brief The OpenGL handle associated with the buffer that this object's instance data has been storerd on.
         * 
         */
        GLuint mVertexBufferIndex {0};

    private:
        /**
         * @brief Deallocates instance data from GPU, deletes the associated vertex buffer.
         * 
         */
        void unload();

        /**
         * @brief Method which calls the override for upload() defined by a subclass.
         * 
         * @todo I'm actually at a loss as to what I was trying to accomplish here.
         * 
         */
        void _upload();

        /**
         * @brief Sets attribute pointers per the data contained in the instance layout.
         * 
         * @param shaderInstanceLayout The layout for instance attributes as described by the shader.
         * @param startingOffset 
         */
        void setAttributePointers(const InstanceLayout& shaderInstanceLayout, std::size_t startingOffset = 0);

        /**
         * @brief The layout associated with this allocator.
         * 
         */
        InstanceLayout mInstanceLayout;

        /**
         * @brief Whether or not the instance data associated with this allocator has been uploaded.
         * 
         */
        bool mUploaded {false};
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The layout of the in-built model matrix instance attribute, present on pretty much every engine-defined shader.
     * 
     */
    static InstanceLayout BuiltinModelMatrixLayout {
        {
            {"modelMatrixCol0", FIXED_MATRIXMODEL, 4, GL_FLOAT},
            {"modelMatrixCol1", FIXED_MATRIXMODEL+1, 4, GL_FLOAT},
            {"modelMatrixCol2", FIXED_MATRIXMODEL+2, 4, GL_FLOAT},
            {"modelMatrixCol3", FIXED_MATRIXMODEL+3, 4, GL_FLOAT},
        }
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief An instance allocator initialized with the built in model matrix layout object.
     * 
     * @see BuiltinModelMatrixLayout
     * 
     * @todo I think a generalization of BuiltinModelMatrixAllocator::upload() is possible and should be explored at some point.
     * 
     */
    class BuiltinModelMatrixAllocator : public BaseInstanceAllocator {
    public:
        BuiltinModelMatrixAllocator(std::vector<glm::mat4> modelMatrices)
        : 
            BaseInstanceAllocator{ BuiltinModelMatrixLayout },
            mModelMatrices{modelMatrices}
        {}

    protected:
        virtual void upload() override;

    private:
        std::vector<glm::mat4> mModelMatrices;
    };
}

#endif
