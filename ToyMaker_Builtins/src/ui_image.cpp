
#include <nlohmann/json.hpp>

#include "toymaker/engine/core/resource_database.hpp"
#include "toymaker/engine/shapegen.hpp"

#include "toymaker/builtins/ui_image.hpp"

using namespace ToyMaker;

std::shared_ptr<ToyMaker::BaseSimObjectAspect> UIImage::create(const nlohmann::json& jsonAspectProperties) {
    std::shared_ptr<UIImage> imageAspect { std::make_shared<UIImage>() };
    const std::string imageFilepath { 
        jsonAspectProperties.at("image_filepath").get<std::string>()
    };

    const glm::vec2 anchor {
        jsonAspectProperties.find("anchor") != jsonAspectProperties.end()?
        glm::vec2{
            jsonAspectProperties.at("anchor")[0].get<float>(),
            jsonAspectProperties.at("anchor")[1].get<float>()
        }:
        glm::vec2{.5f, .5f}
    };

    const glm::uvec2 dimensions {
        jsonAspectProperties.at("dimensions")[0].get<uint32_t>(),
        jsonAspectProperties.at("dimensions")[1].get<uint32_t>(),
    };

    imageAspect->mImageFilepath = imageFilepath;
    imageAspect->mAnchor = anchor;
    imageAspect->mDimensions = dimensions;

    return imageAspect;
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> UIImage::clone() const {
    std::shared_ptr<UIImage> imageAspect { std::make_shared<UIImage>() };
    imageAspect->mImageFilepath = mImageFilepath;
    imageAspect->mAnchor = mAnchor;
    imageAspect->mDimensions = mDimensions;
    return imageAspect;
}

void UIImage::onActivated() {
    recomputeTexture();
}

void UIImage::updateAnchor(const glm::vec2& anchor) {
    if(anchor == mAnchor) return;
    mAnchor = anchor;
    recomputeTexture();
}

void UIImage::updateDimensions(const glm::uvec2& dimensions) {
    if(mDimensions == dimensions) return;
    mDimensions = dimensions;
    recomputeTexture();
}

void UIImage::updateImage(const std::string& imageFilepath) {
    if(mImageFilepath == imageFilepath) return;
    mImageFilepath = imageFilepath;
    recomputeTexture();
}

void UIImage::recomputeTexture() {
    // if no image or dimensions specified, remove associated model
    if(mImageFilepath.empty() || (mDimensions.length() == 0)) {
        if(hasComponent<std::shared_ptr<ToyMaker::StaticModel>>()) {
            removeComponent<std::shared_ptr<ToyMaker::StaticModel>>();
        }
        return;
    }

    // load the image texture
    std::shared_ptr<ToyMaker::Texture> imageTexture {
        ToyMaker::ResourceDatabase::ConstructAnonymousResource<ToyMaker::Texture>({
            {"type", ToyMaker::Texture::getResourceTypeName()},
            {"method", ToyMaker::TextureFromFile::getResourceConstructorName()},
            {"parameters", {
                {"path", mImageFilepath},
            }},
        })
    };

    // compute image dimensions (constrained to container dimensions)
    const glm::vec2 imageDimensions {imageTexture->getWidth(), imageTexture->getHeight()};
    const float imageAspect { imageDimensions.x/imageDimensions.y };
    const float containerAspect { static_cast<float>(mDimensions.x) / mDimensions.y };
    const glm::vec2 imageContainerRatios {
        static_cast<float>(mDimensions.x) / imageDimensions.x,
        static_cast<float>(mDimensions.y) / imageDimensions.y
    };
    const glm::vec2 finalRectangleDimensions {
        imageAspect >= containerAspect?
            glm::vec2 { mDimensions.x, imageDimensions.y * imageContainerRatios.x }:
            glm::vec2 { imageDimensions.x * imageContainerRatios.y, mDimensions.y }
    };

    // create a rectangle model for the texture to be displayed on, and attach
    // it to this node
    const nlohmann::json rectangleParameters = { 
        {"type", ToyMaker::StaticModel::getResourceTypeName()},
        {"method", ToyMaker::StaticModelRectangleDimensions::getResourceConstructorName()},
        {"parameters", {
            {"width", finalRectangleDimensions.x }, {"height", finalRectangleDimensions.y },
            {"flip_texture_y", true},
            {"material_properties", nlohmann::json::array()},
        }}
    };
    if(!hasComponent<std::shared_ptr<ToyMaker::StaticModel>>()) {
        addComponent<std::shared_ptr<ToyMaker::StaticModel>>(
            ToyMaker::ResourceDatabase::ConstructAnonymousResource<ToyMaker::StaticModel>(rectangleParameters)
        );
    } else {
        updateComponent<std::shared_ptr<ToyMaker::StaticModel>>(
            ToyMaker::ResourceDatabase::ConstructAnonymousResource<ToyMaker::StaticModel>(rectangleParameters)
        );
    }

    // shift vertices as specified by the anchors
    std::shared_ptr<ToyMaker::StaticModel>  rectangle { getComponent<std::shared_ptr<ToyMaker::StaticModel>>() };
    for(auto mesh: rectangle->getMeshHandles()) {
        for(auto iVertex{ mesh->getVertexListBegin() }, end {mesh->getVertexListEnd()}; iVertex != end; ++iVertex) {
            iVertex->mPosition += glm::vec4{
                mDimensions.x * (.5f - mAnchor.x),
                mDimensions.y * (mAnchor.y - .5f),
                0.f,
                0.f,
            };
        }
    }
    updateComponent<std::shared_ptr<ToyMaker::StaticModel>>(rectangle);

    // apply the texture to the rectangle
    std::shared_ptr<ToyMaker::Material> material { getComponent<std::shared_ptr<ToyMaker::StaticModel>>()->getMaterialHandles()[0] };
    material->updateTextureProperty(
        "textureAlbedo",
        imageTexture 
    );
    material->updateIntProperty("usesTextureAlbedo", true);
}
