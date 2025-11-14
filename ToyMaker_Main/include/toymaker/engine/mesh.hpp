/**
 * @ingroup ToyMakerRenderSystem
 * @file mesh.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief A file containing the ToyMaker::StaticMesh class and related structures.
 * @version 0.3.2
 * @date 2025-09-03
 * 
 * 
 */

#ifndef FOOLSENGINE_MESH_H
#define FOOLSENGINE_MESH_H

#include <vector>
#include <map>
#include <queue>
#include <memory>

#include <GL/glew.h>

#include <assimp/scene.h>

#include "core/resource_database.hpp"
#include "vertex.hpp"

namespace ToyMaker {
    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A class whose current main purpose is to store geometry related info, and to upload it to GPU memory when requested
     * 
     */
    class StaticMesh: public Resource<StaticMesh> {
    public:
        /**
         * @brief Constructs a new static mesh object
         * 
         * @param mVertices A list of vertex data.
         * @param mElements A list where each item is an index into the vertices array, and where every 3 contiguous elements describe a triangle of a mesh.
         * @param vertexBuffer The OpenGL vertex buffer handle, if one exists, associated with this object in GPU memory.
         * @param elementBuffer The OpenGL element buffer handle, if one exists, associated with this object in GPU memory.
         * @param isUploaded Whether this mesh's data has already been uploaded to the GPU.
         */
        StaticMesh(const std::vector<BuiltinVertexData>& mVertices, const std::vector<GLuint>& mElements, GLuint vertexBuffer=0, GLuint elementBuffer=0, bool isUploaded=false);

        /** @brief Copy constructor. */
        StaticMesh(const StaticMesh& other);

        /** @brief Copy assignment operator */
        StaticMesh& operator=(const StaticMesh& other);

        /** @brief Move constructor */
        StaticMesh(StaticMesh&& other);

        /** @brief Move assignment operator */
        StaticMesh& operator=(StaticMesh&& other);

        /** @brief Destructor */
        ~StaticMesh();

        /**
         * @brief Returns number of elements in the elements array for this mesh.
         * 
         * @return GLuint The no. of elements.
         */
        GLuint getElementCount() {
            return mElements.size();
        }

        /**
         * @brief Binds this object's vertex data according to the vertex layout specified by the shader program.
         * 
         * @param shaderVertexLayout The requested vertex layout.
         */
        void bind(const VertexLayout& shaderVertexLayout);

        /**
         * @brief Unbinds this object's vertex data.
         * 
         */
        void unbind();

        /**
         * @brief Gets an iterator to the beginning of this object's vertex list.
         * 
         * @return std::vector<BuiltinVertexData>::iterator This object's vertex list's beginning iterator.
         */
        std::vector<BuiltinVertexData>::iterator getVertexListBegin();

        /**
         * @brief Gets an iterator to the end of this object's vertex list.
         * 
         * @return std::vector<BuiltinVertexData>::iterator This object's vertex list's ending iterator.
         */
        std::vector<BuiltinVertexData>::iterator getVertexListEnd();

        /**
         * @brief Gets a const iterator to the beginning of this object's vertex list.
         * 
         * @return std::vector<BuiltinVertexData>::const_iterator This object's vertex list's beginning const iterator.
         */
        std::vector<BuiltinVertexData>::const_iterator getVertexListBegin() const;

        /**
         * @brief Gets a const iterator to the end of this object's vertex list.
         * 
         * @return std::vector<BuiltinVertexData>::const_iterator This object's vertex lists' ending const iterator.
         */
        std::vector<BuiltinVertexData>::const_iterator getVertexListEnd() const;

        /**
         * @brief Gets the vertex layout associated with this object, that of BuiltinVertexData.
         * 
         * @return VertexLayout This mesh's vertex layout.
         */
        VertexLayout getVertexLayout() const;

        /**
         * @brief Gets the resource type string for this object.
         * 
         * @return std::string This object's resource type string.
         */
        inline static std::string getResourceTypeName() { return "StaticMesh"; }


    private:

        /**
         * @brief Specifies the offsets of vertex attributes per the vertex layout requested by a shader.
         * 
         * @param shaderVertexLayout The vertex layout required by a shader.
         * @param startingOffset The starting offset, in bytes, to the part of vertex and element arrays associated with this mesh.
         */
        void setAttributePointers(const VertexLayout& shaderVertexLayout, std::size_t startingOffset=0);

        /**
         * @brief This object's vertex data.
         * 
         */
        std::vector<BuiltinVertexData> mVertices {};
        /**
         * @brief This object's element list.
         * 
         * Each element corresponds to an index into mVertices.  Every 3 elements defines one triangle of this mesh.
         * 
         */
        std::vector<GLuint> mElements {};

        /**
         * @brief The vertex layout associated with this mesh, same as BuiltinVertexData's vertex layout.
         * 
         */
        VertexLayout mVertexLayout;

        /**
         * @brief Marker for whether the data in this object has been uploaded to the GPU.
         * 
         */
        bool mIsUploaded { false };

        /**
         * @brief The OpenGL vertex buffer handle for this object, if it has been uploaded to GPU memory.
         * 
         */
        GLuint mVertexBuffer { 0 };

        /**
         * @brief The OpenGL element buffer handle for this object, if it has been uploaded to GPU memory.
         * 
         */
        GLuint mElementBuffer { 0 };

        /**
         * @brief Uploads the vertex and element data for this object to the GPU.
         * 
         */
        void upload();

        /**
         * @brief Deallocates vertex and element data belonging to this object from the GPU.
         * 
         */
        void unload();

        /**
         * @brief Deallocates related GPU buffers and releases resources owned by this object.
         * 
         */
        void destroyResource();

        /**
         * @brief Loses references to resources owned by this object, allowing another object to take ownership of them (including GPU buffers)
         * 
         */
        void releaseResource();
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief Creates a static mesh based on its description in JSON.
     * 
     * Such a representation might look like:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "MyMesh",
     *     "type": "StaticMesh",
     *     "method": "fromDescription",
     *     "parameters": {
     *         "vertices": [
     *             {
     *                 "position": [-1, -1, 0, 1],
     *                 "normal": [0, 0, 1, 0],
     *                 "tangent": [1, 0, 0, 0],
     *                 "color": [1, 1, 1, 1],
     *                 "uv1": [0, 0],
     *                 "uv2": [0, 0],
     *                 "uv3": [0, 0],
     *             },
     *             {
     *                 "position": [1, -1, 0, 1],
     *                 "normal": [0, 0, 1, 0],
     *                 "tangent": [1, 0, 0, 0],
     *                 "color": [1, 1, 1, 1],
     *                 "uv1": [1, 0],
     *                 "uv2": [1, 0],
     *                 "uv3": [1, 0],
     *             },
     *             {
     *                 "position": [1, 1, 0, 1],
     *                 "normal": [0, 0, 1, 0],
     *                 "tangent": [1, 0, 0, 0],
     *                 "color": [1, 1, 1, 1],
     *                 "uv1": [1, 1],
     *                 "uv2": [1, 1],
     *                 "uv3": [1, 1],
     *             }
     *         ],
     *         "elements": [0, 1, 2]
     *     }
     * }
     * 
     * ```
     * 
     */
    class StaticMeshFromDescription: public ResourceConstructor<StaticMesh, StaticMeshFromDescription> {
    public:

        /**
         * @brief Creates this StaticMesh constructor.
         * 
         */
        StaticMeshFromDescription():
        ResourceConstructor<StaticMesh, StaticMeshFromDescription>{0}
        {}

        /**
         * @brief Gets the constructor type string for this constructor.
         * 
         * @return std::string This object's constructor type string.
         * 
         */
        inline static std::string getResourceConstructorName(){ return "fromDescription"; }

    private:

        /**
         * @brief The method responsible for actually creating a StaticMesh.
         * 
         * @param methodParameters The StaticMesh's JSON description.
         * 
         * @return std::shared_ptr<IResource> A handle to the constructed StaticMesh resource.
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };
}

#endif
