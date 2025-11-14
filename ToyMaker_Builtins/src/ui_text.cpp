
#include <nlohmann/json.hpp>

#include "toymaker/engine/core/resource_database.hpp"
#include "toymaker/engine/shapegen.hpp"

#include "toymaker/builtins/ui_text.hpp"

std::shared_ptr<ToyMaker::BaseSimObjectAspect> UIText::create(const nlohmann::json& jsonAspectProperties) {
    const std::string text { 
        jsonAspectProperties.find("text") != jsonAspectProperties.end()?
            jsonAspectProperties.at("text").get<std::string>():
            "Default Text"
    };
    const std::string fontResourceName {
        jsonAspectProperties.find("font_resource_name") != jsonAspectProperties.end()?
            jsonAspectProperties.at("font_resource_name").get<std::string>():
            "DefaultFont"
    };
    const float scale {
        jsonAspectProperties.find("scale") != jsonAspectProperties.end()?
            jsonAspectProperties.at("scale").get<float>():
            .01f
    };
    const glm::vec2 anchor {
        jsonAspectProperties.find("anchor") != jsonAspectProperties.end()?
        glm::vec2{
            jsonAspectProperties.at("anchor")[0].get<float>(),
            jsonAspectProperties.at("anchor")[1].get<float>()
        }:
        glm::vec2{.5f, .5f}
    };
    const glm::u8vec4 color {
        jsonAspectProperties.find("color") != jsonAspectProperties.end()?
        glm::u8vec4 {
            jsonAspectProperties.at("color")[0].get<float>(),
            jsonAspectProperties.at("color")[1].get<float>(),
            jsonAspectProperties.at("color")[2].get<float>(),
            jsonAspectProperties.at("color")[3].get<float>(),
        }:
        glm::u8vec4 { 0x00, 0x00, 0x00, 0xFF }
    };
    const uint32_t maxWidth {
        jsonAspectProperties.find("max_width") != jsonAspectProperties.end()?
        jsonAspectProperties.at("max_width").get<uint32_t>():
        0
    };
    std::shared_ptr<ToyMaker::TextFont> font { ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::TextFont>(fontResourceName) };
    std::shared_ptr<UIText> testTextAspect { std::make_shared<UIText>() };
    testTextAspect->mFont = font;
    testTextAspect->mText = text;
    testTextAspect->mScale = scale;
    testTextAspect->mAnchor = anchor;
    testTextAspect->mColor = color;
    testTextAspect->mMaxWidthPixels = maxWidth;
    return testTextAspect;
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> UIText::clone() const {
    std::shared_ptr<UIText> testTextAspect { std::make_shared<UIText>() };
    testTextAspect->mFont = mFont;
    testTextAspect->mText = mText;
    testTextAspect->mScale = mScale;
    testTextAspect->mAnchor = mAnchor;
    testTextAspect->mColor = mColor;
    testTextAspect->mMaxWidthPixels = mMaxWidthPixels;
    return testTextAspect;
}

void UIText::onActivated() {
    recomputeTexture();
}

void UIText::updateScale(float scale) {
    if(mScale == scale) return;
    mScale = scale;
    recomputeTexture();
}

void UIText::updateText(const std::string& text) {
    if(mText == text) return;
    mText = text;
    recomputeTexture();
}

void UIText::updateFont(const std::string& fontResourceName) {
    std::shared_ptr<ToyMaker::TextFont> font {
        ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::TextFont>(fontResourceName)
    };
    if(font == mFont) return;
    mFont = font;
    recomputeTexture();
}

void UIText::updateAnchor(glm::vec2 anchor) {
    if(anchor == mAnchor) return;
    mAnchor = anchor;
    recomputeTexture();
}

void UIText::updateColor(glm::u8vec4 color) {
    if(color == mColor) return;
    mColor = color;
    recomputeTexture();
}

void UIText::recomputeTexture() {
    std::shared_ptr<ToyMaker::Texture> textTexture { mFont->renderTextArea(mText, mColor, mMaxWidthPixels) };
    const glm::vec2 textDimensions { textTexture->getWidth() * mScale, textTexture->getHeight() * mScale };
    const nlohmann::json rectangleParameters = { 
        {"type", ToyMaker::StaticModel::getResourceTypeName()},
        {"method", ToyMaker::StaticModelRectangleDimensions::getResourceConstructorName()},
        {"parameters", {
            {"width", textDimensions.x }, {"height", textDimensions.y },
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
    std::shared_ptr<ToyMaker::StaticModel>  rectangle { getComponent<std::shared_ptr<ToyMaker::StaticModel>>() };
    for(auto mesh: rectangle->getMeshHandles()) {
        for(auto iVertex{ mesh->getVertexListBegin() }, end {mesh->getVertexListEnd()}; iVertex != end; ++iVertex) {
            iVertex->mPosition += glm::vec4{
                textDimensions.x * (.5f - mAnchor.x),
                textDimensions.y * (mAnchor.y - .5f),
                0.f,
                0.f,
            };
        }
    }
    updateComponent<std::shared_ptr<ToyMaker::StaticModel>>(rectangle);
    std::shared_ptr<ToyMaker::Material> material { getComponent<std::shared_ptr<ToyMaker::StaticModel>>()->getMaterialHandles()[0] };
    material->updateTextureProperty(
        "textureAlbedo",
        textTexture
    );
    material->updateIntProperty("usesTextureAlbedo", true);
}
