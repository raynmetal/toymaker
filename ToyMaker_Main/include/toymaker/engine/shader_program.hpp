/**
 * @ingroup ToyMakerRenderSystem
 * @file shader_program.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief A file containing class definitions for wrappers over OpenGL shader programs.
 * @version 0.3.2
 * @date 2025-09-07
 * 
 * 
 */

#ifndef FOOLSENGINE_SHADERPROGRAM_H
#define FOOLSENGINE_SHADERPROGRAM_H

#include <string>
#include <map>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "core/resource_database.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A wrapper over OpenGL's shader programs.
     * 
     */
    class ShaderProgram: public Resource<ShaderProgram> {
    public:
        /**
         * @brief The resource type string associated with this class.
         * 
         * @return std::string This class' resource type string.
         */
        inline static std::string getResourceTypeName() { return "ShaderProgram"; }

        /**
         * @brief Constructs a new shader program out of an OpenGL shader program already present in memory.
         * 
         * @param program The ID of the OpenGL program.
         */
        ShaderProgram(GLuint program);

        /**
         * @brief Shader destructor; deletes shader from memory
         */
        ~ShaderProgram();


        ShaderProgram(const ShaderProgram& other) = delete;
        ShaderProgram& operator=(const ShaderProgram& other) = delete;

        /** 
         * @brief Shader move constructor 
         */
        ShaderProgram(ShaderProgram&& other) noexcept;
        /** 
         * @brief Shader move assignment
         */
        ShaderProgram& operator=(ShaderProgram&& other) noexcept;

        /** 
         * @brief Makes this shader the active one in this program's OpenGL context.
         */
        void use() const;

        /**
         * @brief Retrieves the location (ID) of an attrib array with a given name.
         * 
         * @param name The name of the attrib array.
         * @return GLint The ID of the attrib array.
         */
        GLint getLocationAttribArray(const std::string& name) const;

        /**
         * @brief Enables an attrib array with a given name.
         * 
         * @param name The name of the array being enabled.
         */
        void enableAttribArray(const std::string& name) const;

        /**
         * @brief Enables an attrib array with a given ID.
         * 
         * @param locationAttrib The ID of the array being enabled.
         */
        void enableAttribArray(GLint locationAttrib) const;

        /**
         * @brief Disables an attrib array with a given name.
         * 
         * @param name The name of the attrib array being disabled.
         */
        void disableAttribArray(const std::string& name) const;

        /**
         * @brief Disables an attrib array with a given ID.
         * 
         * @param locationAttrib The ID of the attrib array being disabled.
         */
        void disableAttribArray(GLint locationAttrib) const;

        /**
         * @brief Gets the location (ID) of a uniform with a given name.
         * 
         * @param name The name of the uniform location being retrieved.
         * @return GLint The ID of the uniform.
         */
        GLint getLocationUniform(const std::string& name) const;

        /**
         * @brief Sets a uniform boolean value.
         * 
         * @param name The name of the boolean uniform.
         * @param value The value of the uniform.
         */
        void setUBool(const std::string& name, bool value) const;

        /**
         * @brief Sets an int uniform's value.
         * 
         * @param name The name of the int uniform.
         * @param value The value of the uniform.
         */
        void setUInt(const std::string& name, int value) const;

        /**
         * @brief Sets a float uniform's value.
         * 
         * @param name The name of the float uniform.
         * @param value The value of the uniform.
         */
        void setUFloat(const std::string& name, float value) const;

        /**
         * @brief Sets a vec2 uniform's value.
         * 
         * @param name The name of the vec2 uniform.
         * @param value The value of the uniform.
         */
        void setUVec2(const std::string& name, const glm::vec2& value) const;

        /**
         * @brief Sets a vec3 uniform's value.
         * 
         * @param name The name of the vec3 uniform.
         * @param value The value of the uniform.
         */
        void setUVec3(const std::string& name, const glm::vec3& value) const;

        /**
         * @brief Sets a vec4 uniform's value.
         * 
         * @param name The name of the vec4 uniform.
         * @param value The value of the uniform.
         */
        void setUVec4(const std::string& name, const glm::vec4& value) const;

        /**
         * @brief Sets a mat4 uniform's value.
         * 
         * @param name The name of the mat4 uniform.
         * @param value The value of the uniform.
         */
        void setUMat4(const std::string& name, const glm::mat4& value) const;

        /**
         * @brief Gets the ID of one of this shader's uniform blocks with a certain name.
         * 
         * This ID can then be set to a UBO binding point, which lets the program find the associated uniform buffer.
         * 
         * @param name The name of this shader's uniform block.
         * @return GLint The ID of the shader's uniform block.
         */
        GLint getLocationUniformBlock(const std::string& name) const;

        /**
         * @brief Binds this shader's named uniform block to a specific uniform binding point.
         * 
         * The binding point lets the shader program find the uniform buffer associated with the binding point via the uniform variable naming the uniform block.
         * 
         * @param name The name of the uniform block.
         * @param bindingPoint The uniform binding point the block should be bound to.
         */
        void setUniformBlock(const std::string& name, GLuint bindingPoint) const;

        /**
         * @brief Returns the ID of the OpenGL shader program this class is a wrapper over.
         * 
         * @return GLuint The ID of this object's OpenGL shader program.
         */
        GLuint getProgramID() const;

    private:
        /**
         * @brief Destroys the shader resource associated with this object.
         * 
         */
        void destroyResource();

        /**
         * @brief Releases the shader resource associated with this object, assuming that it will be picked up by another object or part of the program.
         * 
         */
        void releaseResource();

        /**
         * @brief The ID of the shader program this object is a wrapper over.
         * 
         */
        GLuint mID;
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief Constructs a ShaderProgram from its shader program header, found at a particular path.
     * 
     * ### The resource description for a shader program:
     * 
     * ```c++
     * 
     * nlohmann::json shaderDescription {
     *     {"name", shaderFilepath},
     *     {"type", ShaderProgram::getResourceTypeName()},
     *     {"method", ShaderProgramFromFile::getResourceConstructorName()},
     *     {"parameters", {
     *         {"path", shaderFilepath}
     *     }}
     * }
     * 
     * ```
     * 
     * ### The shader program json header itself:
     * 
     * ```jsonc
     * {
     *     "name": "basicShader",
     *     "type": "shader/program",
     *     "vertexShader": "basicVS.json",
     *     "fragmentShader": "basicFS.json"
     * }
     *
     * ```
     * 
     * ### Fragment shader header:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "basicFS",
     *     "type": "shader/fragment",
     *     "sources": [
     *         "common/versionHeader.glsl",
     *         "common/fragmentAttributes.fs",
     *         "common/genericSampler.fs",
     *         "basic/basic.fs"
     *     ]
     * }
     *
     * ```
     * 
     * ### Vertex shader header:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "basicVS",
     *     "type": "shader/vertex",
     *     "sources": [
     *         "common/versionHeader.glsl",
     *         "common/fragmentAttributes.vs",
     *         "common/vertexAttributes.vs",
     *         "basic/basic.vs"
     *     ]
     * }
     * 
     * ```
     * 
     * ### Vertex shader source (concatenated from shader header `sources` list)
     * 
     * ```c
     * 
     * // "common/versionHeader.glsl"
     * #version 330 core
     * 
     * // "common/vertexAttributes.vs"
     * layout(location=0) in vec4 attrPosition;
     * layout(location=1) in vec4 attrNormal;
     * layout(location=2) in vec4 attrTangent;
     * layout(location=3) in vec4 attrColor;
     * layout(location=4) in vec2 attrUV1;
     * layout(location=5) in vec2 attrUV2;
     * layout(location=6) in vec2 attrUV3;
     * 
     * // "common/fragmentAttributes.vs"
     * out FRAGMENT_ATTRIBUTES {
     *     // world position of the associated fragment
     *     vec4 position;
     *     // normal vector for the associated fragment
     *     vec4 normal;
     *     // tangent vector for the associated fragment
     *     vec4 tangent;
     *     // color of the associated fragment
     *     vec4 color;
     *     // first texture sampling coordinates
     *     vec2 UV1;
     *     // second (ditto)
     *     vec2 UV2;
     *     // third (ditto)
     *     vec2 UV3;
     * } fragAttr;
     * 
     * 
     * // "basic/basic.vs"
     * void main() {
     *     fragAttr.position = attrPosition;
     *     fragAttr.UV1 = attrUV1;
     * 
     *     gl_Position = fragAttr.position;
     * }
     * 
     * ```
     * 
     * Source files may be perused for further examples.
     * 
     */
    class ShaderProgramFromFile: public ResourceConstructor<ShaderProgram, ShaderProgramFromFile> {
    public:
        ShaderProgramFromFile():
        ResourceConstructor<ShaderProgram, ShaderProgramFromFile> {0}
        {}
        inline static std::string getResourceConstructorName() { return "fromFile"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

}

#endif
