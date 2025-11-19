#include "toymaker/engine/core/resource_database.hpp"

#include "toymaker/engine/shapegen.hpp"
#include "toymaker/engine/framebuffer.hpp"

#include "toymaker/builtins/nine_slice_panel.hpp"

using namespace ToyMaker;

NineSlicePanel::NineSlicePanel(
    std::shared_ptr<ToyMaker::Texture> baseTexture,
    SDL_FRect contentRegionUV
):
ToyMaker::Resource<NineSlicePanel>{0},
mTexture { baseTexture },
mContentRegion { contentRegionUV }
{
    assert(mTexture != nullptr && "NineSlice base texture should be a valid texture");
    assert(
        contentRegionUV.x >= 0.f && contentRegionUV.y >= 0.f 
        && contentRegionUV.x <= 1.f && contentRegionUV.y <= 1.f
        && "Content region's start must be within the bounds of the texture"
    );
    assert(
        contentRegionUV.w >= 0.f && contentRegionUV.x + contentRegionUV.w <= 1.f
        && contentRegionUV.h >= 0.f && contentRegionUV.y + contentRegionUV.h <= 1.f
        && "Content region's end must be within the bounds of the texture"
    );
    const std::string nineSliceShaderPath { "data/shader/nineSliceShader.json" };
    if(!ToyMaker::ResourceDatabase::HasResourceDescription(nineSliceShaderPath)){
        nlohmann::json shaderDescription {
            {"name", nineSliceShaderPath},
            {"type", ToyMaker::ShaderProgram::getResourceTypeName()},
            {"method", ToyMaker::ShaderProgramFromFile::getResourceConstructorName()},
            {"parameters", {
                {"path", nineSliceShaderPath}
            }}
        };
        ToyMaker::ResourceDatabase::AddResourceDescription(shaderDescription);
    }
    mShaderHandle = ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::ShaderProgram>(nineSliceShaderPath);
    glGenVertexArrays(1, &mVertexArrayObject);
    if(!ToyMaker::ResourceDatabase::HasResourceDescription("screenRectangleMesh")) {
        const nlohmann::json rectangleMeshDefinition {
            {"name", "screenRectangleMesh"},
            {"type", ToyMaker::StaticMesh::getResourceTypeName()},
            {"method", ToyMaker::StaticMeshRectangleDimensions::getResourceConstructorName()},
            {"parameters", {
                {"width", 2.f},
                {"height", 2.f},
            }}
        };
        ToyMaker::ResourceDatabase::AddResourceDescription(rectangleMeshDefinition);
    }
}

NineSlicePanel::~NineSlicePanel() {
    if(mVertexArrayObject) {
        glDeleteVertexArrays(1, &mVertexArrayObject);
    }
}

std::shared_ptr<ToyMaker::Texture> NineSlicePanel::generateTexture(glm::uvec2 contentDimensions) const {
    const glm::uvec2 targetDimensions {
        contentDimensions + glm::uvec2 {
            getOffsetPixelLeft() + getOffsetPixelRight(),
            getOffsetPixelTop() + getOffsetPixelBottom()           
        }
    };
    const nlohmann::json framebufferDescription {
        {"type", ToyMaker::Framebuffer::getResourceTypeName()},
        {"method", ToyMaker::FramebufferFromDescription::getResourceConstructorName()},
        {"parameters", {
            {"nColorAttachments", 1},
            {"dimensions", {targetDimensions.x, targetDimensions.y}},
            {"ownsRBO", false},
            {"colorBufferDefinitions", {
                ToyMaker::ColorBufferDefinition {
                    .mDimensions{targetDimensions.x, targetDimensions.y},
                    .mDataType=GL_FLOAT,
                    .mComponentCount=4,
                }
            }}
        }},
    };
    std::shared_ptr<ToyMaker::Framebuffer> framebuffer {
        ToyMaker::ResourceDatabase::ConstructAnonymousResource<ToyMaker::Framebuffer>(
            framebufferDescription
        )
    };
    std::shared_ptr<ToyMaker::StaticMesh> rectangleMesh {
        ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::StaticMesh>(
            "screenRectangleMesh"
        )
    };

    mShaderHandle->use();

    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, targetDimensions.x, targetDimensions.y);
    framebuffer->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        mTexture->bind(0);
        mShaderHandle->setUInt("uGenericTexture", 0);
        mShaderHandle->setUVec2("uPanelTextureDimensions", mTexture->getColorBufferDefinition().mDimensions);
        mShaderHandle->setUVec2("uTargetTextureDimensions", targetDimensions);
        mShaderHandle->setUVec2("uPanelContentUVStart", glm::vec2{mContentRegion.x, mContentRegion.y});
        mShaderHandle->setUVec2("uPanelContentUVEnd", glm::vec2{
            mContentRegion.x + mContentRegion.w,
            mContentRegion.y + mContentRegion.h
        });

        glBindVertexArray(mVertexArrayObject);
            rectangleMesh->bind({{
                {"position", ToyMaker::LOCATION_POSITION, 4, GL_FLOAT},
                {"UV1", ToyMaker::LOCATION_UV1, 2, GL_FLOAT},
            }});
            glDrawElementsInstanced(
                GL_TRIANGLES,
                rectangleMesh->getElementCount(),
                GL_UNSIGNED_INT,
                nullptr,
                1
            );
        glBindVertexArray(0);
    framebuffer->unbind();

    return framebuffer->getTargetColorBufferHandles()[0];
};

uint32_t NineSlicePanel::getOffsetPixelLeft() const {
    return (
        mContentRegion.x 
        * mTexture->getWidth()
    );
}
uint32_t NineSlicePanel::getOffsetPixelRight() const {
    return (
        (
            1.f - (
                mContentRegion.x + mContentRegion.w
            )
        ) * mTexture->getWidth()
    );
}
uint32_t NineSlicePanel::getOffsetPixelBottom() const {
    return (
        mContentRegion.y
        * mTexture->getHeight()
    );
}
uint32_t NineSlicePanel::getOffsetPixelTop() const {
    return (
        (
            1.f - (
                mContentRegion.y + mContentRegion.h
            )
        ) * mTexture->getHeight()
    );
}

std::shared_ptr<ToyMaker::IResource> NineSlicePanelFromDescription::createResource(const nlohmann::json& methodParameters) {
    return std::make_shared<NineSlicePanel>(
        ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::Texture>(methodParameters.at("base_texture")),
        SDL_FRect {
            .x { methodParameters.at("content_region")[0] },
            .y { methodParameters.at("content_region")[1] },
            .w { methodParameters.at("content_region")[2] },
            .h { methodParameters.at("content_region")[3] },
        }
    );
}
