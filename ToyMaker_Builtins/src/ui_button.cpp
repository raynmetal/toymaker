#include "toymaker/builtins/ui_text.hpp"
#include "toymaker/builtins/ui_button.hpp"

using namespace ToyMaker;

std::shared_ptr<ToyMaker::BaseSimObjectAspect> UIButton::create(const nlohmann::json& jsonAspectProperties) {
    const glm::vec2 anchor {
        jsonAspectProperties.find("anchor") != jsonAspectProperties.end()?
        glm::vec2{
            jsonAspectProperties.at("anchor")[0].get<float>(),
            jsonAspectProperties.at("anchor")[1].get<float>()
        }:
        glm::vec2{.5f, .5f}
    };
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
    const std::string value {
        jsonAspectProperties.find("value") != jsonAspectProperties.end()?
            jsonAspectProperties.at("value").get<std::string>():
            ""
    };
    const std::string panelActive {
        jsonAspectProperties.at("panel_active").get<std::string>()
    };
    const std::string panelInactive {
        jsonAspectProperties.at("panel_inactive").get<std::string>()
    };
    const std::string panelHover {
        jsonAspectProperties.at("panel_hover").get<std::string>()
    };
    const std::string panelPressed {
        jsonAspectProperties.at("panel_pressed").get<std::string>()
    };
    const bool hasHighlight {
        jsonAspectProperties.at("has_highlight").get<bool>()
    };
    std::string highlight;
    glm::vec4 highlightColor;
    if(hasHighlight) {
        highlight = jsonAspectProperties.at("highlight").get<std::string>();
        highlightColor = glm::vec4{
            jsonAspectProperties.at("highlight_color")[0].get<float>(),
            jsonAspectProperties.at("highlight_color")[1].get<float>(),
            jsonAspectProperties.at("highlight_color")[2].get<float>(),
            jsonAspectProperties.at("highlight_color")[3].get<float>()
        };
    }

    std::shared_ptr<UIButton> buttonAspect { std::make_shared<UIButton>() };
    buttonAspect->mStatePanels[State::INACTIVE] = ToyMaker::ResourceDatabase::GetRegisteredResource<NineSlicePanel>(panelInactive);
    buttonAspect->mStatePanels[State::ACTIVE] = ToyMaker::ResourceDatabase::GetRegisteredResource<NineSlicePanel>(panelActive);
    buttonAspect->mStatePanels[State::PRESSED] = ToyMaker::ResourceDatabase::GetRegisteredResource<NineSlicePanel>(panelPressed);
    buttonAspect->mStatePanels[State::HOVER] = ToyMaker::ResourceDatabase::GetRegisteredResource<NineSlicePanel>(panelHover);
    if(hasHighlight) {
        buttonAspect->mHighlightPanel = ToyMaker::ResourceDatabase::GetRegisteredResource<NineSlicePanel>(highlight);
    }
    buttonAspect->mAnchor = anchor;
    buttonAspect->mTextScaleOverride = scale;
    buttonAspect->mTextOverride = text;
    buttonAspect->mTextFontOverride = fontResourceName;
    buttonAspect->mTextColorOverride = color;
    buttonAspect->mValue = value;
    if(hasHighlight) {
        buttonAspect->mHighlightColor = highlightColor;
    }

    return buttonAspect;
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> UIButton::clone() const {
    std::shared_ptr<UIButton> buttonAspect { std::make_shared<UIButton>() };
    buttonAspect->mStatePanels = mStatePanels;
    buttonAspect->mHighlightPanel = mHighlightPanel;
    buttonAspect->mAnchor = mAnchor;
    buttonAspect->mTextScaleOverride = mTextScaleOverride;
    buttonAspect->mTextOverride = mTextOverride;
    buttonAspect->mTextFontOverride = mTextFontOverride;
    buttonAspect->mTextColorOverride = mTextColorOverride;
    buttonAspect->mValue = mValue;
    buttonAspect->mHighlightColor = mHighlightColor;

    return buttonAspect;
}

void UIButton::onActivated() {
    std::shared_ptr<ToyMaker::SimObject> textNode {getTextObject()};
    UIText& textAspect { textNode->getAspect<UIText>() };
    textAspect.updateText(mTextOverride);
    textAspect.updateFont(mTextFontOverride);
    textAspect.updateScale(mTextScaleOverride);
    textAspect.updateColor(mTextColorOverride);
    recomputeTexture();
    fireStateEvent();
}

void UIButton::enableButton() {
    if(mCurrentState != State::INACTIVE) return;
    updateButtonState(mHovered? State::HOVER: State::ACTIVE);
}
void UIButton::disableButton() {
    if(mCurrentState == State::INACTIVE) return;
    updateButtonState(State::INACTIVE);
}

void UIButton::updateButtonState(UIButton::State state) {
    if(state == mCurrentState) return;
    mCurrentState = state;
    recomputeTexture();
    fireStateEvent();
}

void UIButton::updateButtonAnchor(glm::vec2 anchor) {
    if(mAnchor == anchor) return;
    mAnchor = anchor;
    recomputeTexture();
}

void UIButton::updateText(const std::string& newText) {
    getTextObject()->getAspect<UIText>().updateText(newText);
    recomputeTexture();
}

void UIButton::updateTextScale(float scale) {
    getTextObject()->getAspect<UIText>().updateScale(scale);
    recomputeTexture();
}

void UIButton::updateTextFont(const std::string& textResourceName) {
    getTextObject()->getAspect<UIText>().updateFont(textResourceName);
    recomputeTexture();
}

void UIButton::updateTextColor(glm::u8vec4 textColor) {
    getTextObject()->getAspect<UIText>().updateColor(textColor);
    recomputeTexture();
}

void UIButton::updateHighlightColor(glm::vec4 highlightColor) {
    if(highlightColor == mHighlightColor) return;
    mHighlightColor = highlightColor;
    recomputeTexture();
}

void UIButton::fireStateEvent() {
    switch(mCurrentState) {
        case State::ACTIVE:
            mSigButtonActivated.emit();
            break;
        case State::INACTIVE:
            mSigButtonDeactivated.emit();
            break;
        case State::HOVER:
            mSigButtonHoveredOver.emit(mValue);
            break;
        case State::PRESSED:
            mSigButtonPressed.emit(mValue);
            break;

        default:
            assert(false && "State change to unknown state requested");
            break;
    }
}

bool UIButton::onPointerEnter(glm::vec4 pointerLocation) {
    (void)pointerLocation; // prevent unused parameter warning
    mHovered=true;
    if(mCurrentState == State::INACTIVE) return false;

    updateButtonState(State::HOVER);
    return true;
}

bool UIButton::onPointerLeave() {
    mHovered=false;
    if(mCurrentState == State::INACTIVE) return false;

    updateButtonState(State::ACTIVE);
    return true;
}

bool UIButton::onPointerLeftClick(glm::vec4 pointerLocation) {
    (void)pointerLocation; // prevent unused parameter warning
    if(mCurrentState == State::INACTIVE) return false;
    updateButtonState(State::PRESSED);
    return true;
}

bool UIButton::onPointerLeftRelease(glm::vec4 pointerLocation) {
    (void)pointerLocation; // prevent unused parameter warnings
    if(mCurrentState == State::INACTIVE) return false;

    assert(
        (mCurrentState == State::HOVER || mCurrentState == State::PRESSED)
        && "Button must be hovered on or pressed currently"
    );
    if(mCurrentState != State::PRESSED) {
        return false;
    }

    updateButtonState(State::HOVER);
    mSigButtonReleased.emit(mValue);
    return true;
}

void UIButton::recomputeTexture() {
    std::shared_ptr<NineSlicePanel>& basePanel { mStatePanels[mCurrentState] };

    // see how large the button text is
    std::shared_ptr<ToyMaker::SimObject> textObject { getTextObject() };
    const std::shared_ptr<ToyMaker::Texture> textTexture {
        textObject->getComponent<std::shared_ptr<ToyMaker::StaticModel>>()->getMaterialHandles()[0]
            ->getTextureProperty("textureAlbedo")
    };
    const glm::vec2 contentSize { textTexture->getWidth(), textTexture->getHeight() };

    // compute a new texture for our button panel based on the size of the text
    std::shared_ptr<ToyMaker::Texture> panelTexture{basePanel->generateTexture(contentSize)};
    const glm::vec2 panelSize { panelTexture->getWidth(), panelTexture->getHeight() };
    // update panel mesh as well
    const nlohmann::json rectangleParameters = {
        {"type", ToyMaker::StaticModel::getResourceTypeName()},
        {"method", ToyMaker::StaticModelRectangleDimensions::getResourceConstructorName()},
        {"parameters", {
            {"width", panelSize.x}, {"height", panelSize.y},
            {"flip_texture_y", true},
            {"material_properties", nlohmann::json::array()}
        }}
    };

    getSimObject().addOrUpdateComponent<std::shared_ptr<ToyMaker::StaticModel>>(
            ToyMaker::ResourceDatabase::ConstructAnonymousResource<ToyMaker::StaticModel>(rectangleParameters)
    );

    const glm::vec4 anchorPixelOffset {
        panelSize.x * (.5f - mAnchor.x), panelSize.y * (mAnchor.y - .5f),
        0.f, 0.f
    };
    std::shared_ptr<ToyMaker::StaticModel>  rectangle { getComponent<std::shared_ptr<ToyMaker::StaticModel>>() };
    for(auto mesh: rectangle->getMeshHandles()) {
        for(auto iVertex{ mesh->getVertexListBegin() }, end {mesh->getVertexListEnd()}; iVertex != end; ++iVertex) {
            iVertex->mPosition += anchorPixelOffset;
        }
    }
    updateComponent<std::shared_ptr<ToyMaker::StaticModel>>(rectangle);
    std::shared_ptr<ToyMaker::Material> material { getComponent<std::shared_ptr<ToyMaker::StaticModel>>()->getMaterialHandles()[0] };
    material->updateTextureProperty("textureAlbedo", panelTexture);
    material->updateIntProperty("usesTextureAlbedo", true);

    if(mHighlightPanel) {
        std::shared_ptr<ToyMaker::SceneNode> highlightNode { getSimObject().getByPath("/highlight/") };
        highlightNode->addOrUpdateComponent<std::shared_ptr<ToyMaker::StaticModel>>(
                ToyMaker::ResourceDatabase::ConstructAnonymousResource<ToyMaker::StaticModel>(rectangleParameters)
        );
        std::shared_ptr<ToyMaker::StaticModel> highlightRectangle { highlightNode->getComponent<std::shared_ptr<ToyMaker::StaticModel>>() };
        std::shared_ptr<ToyMaker::Material> highlightMaterial { highlightRectangle->getMaterialHandles()[0] };
        for(auto mesh: highlightRectangle->getMeshHandles()) {
            for(auto iVertex{ mesh->getVertexListBegin() }, end {mesh->getVertexListEnd()}; iVertex != end; ++iVertex) {
                iVertex->mPosition += anchorPixelOffset;
            }
        }
        std::shared_ptr<ToyMaker::Texture> highlightTexture { mHighlightPanel->generateTexture(contentSize) };
        highlightMaterial->updateTextureProperty("textureAlbedo", highlightTexture);
        highlightMaterial->updateIntProperty("usesTextureAlbedo", true);
        highlightMaterial->updateVec4Property("colorMultiplier", mHighlightColor);

        ToyMaker::Placement highlightPlacement {};
        highlightPlacement.mPosition.z += 0.2f;
        highlightNode->updateComponent<ToyMaker::Placement>(highlightPlacement);
    }

    // update text position accordingly
    const glm::vec4 contentCenter { 
        static_cast<float>(basePanel->getOffsetPixelLeft()) + (contentSize.x - panelSize.x)*.5f + anchorPixelOffset.x,
        static_cast<float>(basePanel->getOffsetPixelBottom()) + (contentSize.y - panelSize.y)*.5f + anchorPixelOffset.y,
        .1f,
        1.f
    };
    ToyMaker::Placement textPlacement { textObject->getComponent<ToyMaker::Placement>() };
    textPlacement.mPosition = contentCenter;
    textObject->getAspect<UIText>().updateAnchor({.5f, .5f});
    textObject->updateComponent<ToyMaker::Placement>(textPlacement);
}

std::shared_ptr<ToyMaker::SimObject> UIButton::getTextObject() {
    std::shared_ptr<ToyMaker::SimObject> textNode {
        getSimObject().getByPath<std::shared_ptr<ToyMaker::SimObject>>("/button_text/")
    };
    assert(textNode && textNode->hasAspect<UIText>() && "A node with the UIButton aspect requires a child node with a UIText aspect");
    return textNode;
}
