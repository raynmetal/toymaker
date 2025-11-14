/**
 * @ingroup ToyMakerRenderSystem
 * @file vertex.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains engine's built-in vertex definitions, along with their associated attribute locations in the engine's built-in shader programs.  A wrapper over OpenGL shader attributes.
 * @version 0.3.2
 * @date 2025-09-11
 * 
 */

#ifndef FOOLSENGINE_VERTEX_H
#define FOOLSENGINE_VERTEX_H

#include <string>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace ToyMaker {

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Values for different attribute locations used by the engine's built-in shader programs.
     * 
     */
    enum DefaultAttributeLocation {
        LOCATION_POSITION=0, //< Location usually mapped to a vertex shader's "attrPosition" vec4 attribute.
        LOCATION_NORMAL=1, //< Location usually mapped to a vertex shader's "attrNormal" vec4 attribute.
        LOCATION_TANGENT=2, //< Location usually mapped to a vertex shader's "attrTangent" vec4 attribute.
        LOCATION_COLOR=3, //< Location usually mapped to a vertex shader's "attrColor" vec4 attribute.
        LOCATION_UV1=4, //< Location usually mapped to a vertex shader's "attrUV1" vec2 attribute.
        LOCATION_UV2=5, //< Location usually mapped to a vertex shader's "attrUV2" vec2 attribute.
        LOCATION_UV3=6, //< Location usually mapped to a vertex shader's "attrUV3" vec2 attribute.
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The description of a single vertex attribute associated with a vertex layout, giving its size and location id.
     * 
     */
    struct VertexAttributeDescriptor {
        /**
         * @brief Creates a single vertex attribute descriptor.
         * 
         * @param name The name of the attribute, as seen by the shader.
         * @param layoutLocation The location ID of the attribute, as specified by the shader.
         * @param nComponents The number of components making up this attribute.
         * @param type The type of a single component of the attribute (like GL_FLOAT, GL_UNSIGNED_BYTE, and so on).
         */
        VertexAttributeDescriptor(const std::string& name, GLint layoutLocation, GLuint nComponents=1, GLenum type=GL_FLOAT)
        : 
            mName{name}, mLayoutLocation{layoutLocation}, mNComponents{nComponents}, 
            mType{type}, mSize { GetGLTypeSize(type) * nComponents}
        {
            assert(mNComponents >= 1 && mNComponents <= 4);
            assert(mType == GL_FLOAT || mType == GL_UNSIGNED_INT || mType == GL_INT);
        }

        /**
         * @brief The name of the attribute, per the shader.
         * 
         */
        std::string mName;

        /**
         * @brief The layout location ID of the attribute, as specified by the shader.
         * 
         */
        GLint mLayoutLocation;

        /**
         * @brief The number of components making up the attribute.
         * 
         */
        GLuint mNComponents;

        /**
         * @brief The type of component used by the attribute, such as GL_FLOAT or GL_UNSIGNED_BYTE
         * 
         */
        GLenum mType;

        /**
         * @brief The computed size of the attribute, in bytes.
         * 
         * Given by : size of `mType` * `mNComponents`
         * 
         */
        std::size_t mSize;

        /**
         * @brief Compares two vertex attribute descriptors for equality.
         * 
         * Mainly used to determine whether the vertex layout requested by a render stage matches the vertex layout associated with some vertex data.
         * 
         * @param other The vertex descriptor this one is being compared to.
         * @retval true The other description matches this one.
         * @retval false The other description does not match this one.
         */
        bool operator==(const VertexAttributeDescriptor& other) const {
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
         * @brief Gets the size of the type of a single component of an attribute.
         * 
         * @param type The GLenum of the component type.
         * @return std::size_t The size of that component type, in bytes.
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
                    assert(false && "Unsupported or invalid attribute component type specified");
            }
            return componentSize;
        }
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A list of attribute descriptors that together define the layout and size of the vertex they make up in GPU memory.
     * 
     * @todo This isn't particularly intuitive or useful yet; expand on this so that shaders can declare their vertex layouts in their json headers maybe?  At present we've just hardcoded this into the render stages.
     * 
     */
    struct VertexLayout {
        /**
         * @brief Create a vertex layout from a list of attribute descriptors.
         * 
         * @param attributeList The list of attributes in the layout.
         */
        VertexLayout(std::vector<VertexAttributeDescriptor> attributeList) : mAttributeList{attributeList} {}

        /**
         * @brief Gets the list of attribute descriptors making up this layout.
         * 
         * @return std::vector<VertexAttributeDescriptor> This layout's attribute descriptor.
         */
        std::vector<VertexAttributeDescriptor> getAttributeList() const {
            return mAttributeList;
        }

        /**
         * @brief Computes the total space occupied by a single vertex including all its attributes in memory.
         * 
         * @return std::size_t The total size of a single vertex in memory.
         * 
         */
        std::size_t computeStride() const {
            std::size_t stride {0};
            for(const auto& attribute : mAttributeList) {
                stride += attribute.mSize;
            }
            return stride;
        }

        /**
         * @brief Given the index of an attribute descriptor within the layout, computes the offset to that attribute from the beginning of the vertex, in GPU memory.
         * 
         * @param attributeIndex The index of the attribute whose offset we want.
         * @return std::size_t The number of bytes from the start of the vertex data to the start of the attribute.
         */
        std::size_t computeRelativeOffset(std::size_t attributeIndex) const {
            assert(attributeIndex < mAttributeList.size());
            std::size_t baseOffset {0};
            for(std::size_t i {0}; i < attributeIndex; ++i) {
                baseOffset += mAttributeList[i].mSize;
            }
            return baseOffset;
        }

        /**
         * @brief Tests whether this vertex layout is the subset of another.
         * 
         * This vertex must have attributes in the same order as the other, but may skip any attributes present on the other.
         * 
         * Used mainly to determine whether the layout requested by a RenderStage is compatible with the layout describing some vertex data.
         * 
         * @param other The layout this one is possibly a subset of.
         * @retval true This layout is a subset of the other.
         * @retval false This layout is not a subset of the other.
         */
        bool isSubsetOf(const VertexLayout& other) const {
            if(mAttributeList.size() > other.mAttributeList.size()) return false;

            std::size_t myAttributeIndex {0};
            for(std::size_t i{0}; i < other.mAttributeList.size() && myAttributeIndex < mAttributeList.size(); ++i) {
                if(other.mAttributeList[i] == mAttributeList[myAttributeIndex]){
                    ++myAttributeIndex;
                    if(myAttributeIndex == mAttributeList.size()) return true;
                }
            }

            return false;
        }

    private:
        /**
         * @brief The list of attribute descriptors that define this layout.
         * 
         */
        std::vector<VertexAttributeDescriptor> mAttributeList {};
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The vertex data used by all this engine's in-built shader program's vertex shaders.
     * 
     */
    struct BuiltinVertexData {
        /**
         * @brief The position of the vertex (normally with `w` set to 1.f).
         * 
         */
        glm::vec4 mPosition;

        /**
         * @brief The normal vector to the vertex (with `w` set to 0.f)
         * 
         */
        glm::vec4 mNormal;

        /**
         * @brief The tangent vector to the vertex (with `w` set to 0.f)
         * 
         */
        glm::vec4 mTangent;

        /**
         * @brief This vertex's color, where each color component is a value between 0.f and 1.f.
         * 
         */
        glm::vec4 mColor {1.f}; // by default, white

        /**
         * @brief The UV coordinates corresponding to this vertex, with coordinates pointing into the first texture set used by its model.
         * 
         */
        glm::vec2 mUV1;

        /**
         * @brief (currently unused) Coordinates pointing into the second texture set used by this vertex's owning model.
         * 
         * @see mUV1
         * 
         */
        glm::vec2 mUV2;

        /**
         * @brief (currently unused) Coordinates pointing into the third texture set used by this vertex's owning model.
         * 
         * @see mUV1
         * 
         */
        glm::vec2 mUV3;
    };

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    inline void from_json(const nlohmann::json& json, BuiltinVertexData& builtinVertexData) {
        json.at("position").at(0).get_to(builtinVertexData.mPosition.x);
        json.at("position").at(1).get_to(builtinVertexData.mPosition.y);
        json.at("position").at(2).get_to(builtinVertexData.mPosition.z);
        json.at("position").at(3).get_to(builtinVertexData.mPosition.w);
        json.at("normal").at(0).get_to(builtinVertexData.mNormal.x);
        json.at("normal").at(1).get_to(builtinVertexData.mNormal.y);
        json.at("normal").at(2).get_to(builtinVertexData.mNormal.z);
        json.at("normal").at(3).get_to(builtinVertexData.mNormal.w);
        json.at("tangent").at(0).get_to(builtinVertexData.mTangent.x);
        json.at("tangent").at(1).get_to(builtinVertexData.mTangent.y);
        json.at("tangent").at(2).get_to(builtinVertexData.mTangent.z);
        json.at("tangent").at(3).get_to(builtinVertexData.mTangent.w);
        json.at("color").at(0).get_to(builtinVertexData.mColor.r);
        json.at("color").at(1).get_to(builtinVertexData.mColor.g);
        json.at("color").at(2).get_to(builtinVertexData.mColor.b);
        json.at("color").at(3).get_to(builtinVertexData.mColor.a);
        json.at("uv1").at(0).get_to(builtinVertexData.mUV1.s);
        json.at("uv1").at(1).get_to(builtinVertexData.mUV1.t);
        json.at("uv2").at(0).get_to(builtinVertexData.mUV2.s);
        json.at("uv2").at(1).get_to(builtinVertexData.mUV2.t);
        json.at("uv3").at(0).get_to(builtinVertexData.mUV3.s);
        json.at("uv3").at(1).get_to(builtinVertexData.mUV3.t);
    }

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    inline void to_json(nlohmann::json& json, const BuiltinVertexData& builtinVertexData) {
        json = {
            {"position", 
                { 
                    builtinVertexData.mPosition.x,
                    builtinVertexData.mPosition.y,
                    builtinVertexData.mPosition.z,
                    builtinVertexData.mPosition.w
                }
            },
            {"normal",
                {
                    builtinVertexData.mNormal.x,
                    builtinVertexData.mNormal.y,
                    builtinVertexData.mNormal.z,
                    builtinVertexData.mNormal.w,
                }
            },
            {"tangent",
                {
                    builtinVertexData.mTangent.x,
                    builtinVertexData.mTangent.y,
                    builtinVertexData.mTangent.z,
                    builtinVertexData.mTangent.w,
                }
            },
            {"color",
                {
                    builtinVertexData.mColor.r,
                    builtinVertexData.mColor.g,
                    builtinVertexData.mColor.b,
                    builtinVertexData.mColor.a,
                }
            },
            {"uv1",
                {
                    builtinVertexData.mUV1.s,
                    builtinVertexData.mUV1.t,
                }
            },
            {"uv2",
                {
                    builtinVertexData.mUV2.s,
                    builtinVertexData.mUV2.t,
                }
            },
            {"uv3",
                {
                    builtinVertexData.mUV3.s,
                    builtinVertexData.mUV3.t,
                }
            }
        };
    }

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The vertex layout used by most shader programs built into the engine.
     * 
     */
    inline VertexLayout BuiltinVertexLayout {
        {
            {"position", LOCATION_POSITION, 4, GL_FLOAT},
            {"normal", LOCATION_NORMAL, 4, GL_FLOAT},
            {"tangent", LOCATION_TANGENT, 4, GL_FLOAT},
            {"color", LOCATION_COLOR, 4, GL_FLOAT},
            {"UV1", LOCATION_UV1, 2, GL_FLOAT},
            {"UV2", LOCATION_UV2, 2, GL_FLOAT},
            {"UV3", LOCATION_UV3, 2, GL_FLOAT}
        }
    };

}

#endif
