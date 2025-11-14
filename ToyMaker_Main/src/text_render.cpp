#include <SDL2/SDL_ttf.h>

#include "toymaker/engine/text_render.hpp"

using namespace ToyMaker;

TextFont::TextFont(const std::string& fontPath, uint16_t pointSize):
Resource<TextFont>{0},
mFont { LoadFont(fontPath, pointSize) },
mFontPath { fontPath },
mPointSize { pointSize }
{}

TextFont::~TextFont() {
    if(!mFont) { return; }
    TTF_CloseFont(mFont);
    mFont = nullptr;
}

TTF_Font* TextFont::LoadFont(const std::string& fontPath, uint16_t pointSize) {
    assert(pointSize >= 2 && "Specified point size is nonsensically small");
    TTF_Font* openedFont { TTF_OpenFont(fontPath.c_str(), pointSize) };
    assert(openedFont && "Could not load font with the specified parameters");
    return openedFont;
}

std::shared_ptr<IResource> TextFontFromFile::createResource(const nlohmann::json& methodParameters) {
    const std::string filepath { methodParameters.at("path").get<std::string>() };
    const uint16_t pointSize { methodParameters.at("point_size").get<uint16_t>() };
    return std::make_shared<TextFont>(filepath, pointSize);
}

std::shared_ptr<Texture> TextFont::renderText(
    const std::string& text,
    glm::u8vec3 textColor,
    glm::u8vec3 backgroundColor
) const {
    SDL_Surface* renderedText { TTF_RenderUTF8_Shaded(
        mFont, 
        text.c_str(),
        SDL_Color{
            .r { textColor.r },
            .g { textColor.g },
            .b { textColor.b },
            .a { 0xFF }
        },
        SDL_Color {
            .r { backgroundColor.r },
            .g { backgroundColor.g },
            .b { backgroundColor.b },
            .a { 0xFF }
        }
    )};
    return MakeTexture(renderedText);
}

std::shared_ptr<Texture> TextFont::renderText(const std::string& text, glm::u8vec4 textColor) const {
    SDL_Surface* renderedText { TTF_RenderUTF8_Solid(
        mFont, 
        text.c_str(),
        SDL_Color{
            .r { textColor.r },
            .g { textColor.g },
            .b { textColor.b },
            .a { textColor.a }
        }
    )};

    return MakeTexture(renderedText);
}

std::shared_ptr<Texture> TextFont::renderTextArea(const std::string& text, glm::u8vec4 textColor, uint32_t wrapLength) const {
    SDL_Surface* renderedText {
        TTF_RenderUTF8_Blended_Wrapped(
            mFont,
            text.c_str(),
            SDL_Color {
                .r { textColor.r },
                .g { textColor.g },
                .b { textColor.b },
                .a { textColor.a },
            },
            wrapLength
        )
    };
    if(!renderedText) {
        std::cout << "TTF text rendering error occurred: " << TTF_GetError() << "\n";
    }

    return MakeTexture(renderedText);
}

std::shared_ptr<Texture> TextFont::MakeTexture(SDL_Surface* renderedText) {
    ColorBufferDefinition colorBufferDefinition {
        .mDataType { GL_UNSIGNED_BYTE },
        .mComponentCount { 4 },
        .mUsesWebColors { true }
    };
    SDL_Surface* pretexture { SDL_ConvertSurfaceFormat(renderedText, SDL_PIXELFORMAT_RGBA32, 0) };
    if(!pretexture) {
        std::cout << "SDL surface conversion error: " << SDL_GetError() << "\n";
    }
    SDL_FreeSurface(renderedText);
    renderedText = nullptr;
    assert(pretexture && "Could not convert SDL image to known image format");

    // Move surface pixels to the graphics card
    GLuint texture;
    colorBufferDefinition.mDimensions = {pretexture->w, pretexture->h};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,
        deduceInternalFormat(colorBufferDefinition),
        colorBufferDefinition.mDimensions.x, colorBufferDefinition.mDimensions.y,
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        reinterpret_cast<void*>(pretexture->pixels)
    );
    SDL_FreeSurface(pretexture);
    pretexture = nullptr;

    // Check for OpenGL errors
    GLuint error { glGetError() };
    assert(error == GL_NO_ERROR && "An error occurred during allocation of openGL texture");

    // Configure a few texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, colorBufferDefinition.mWrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, colorBufferDefinition.mWrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, colorBufferDefinition.mMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, colorBufferDefinition.mMagFilter);

    return std::make_shared<Texture>(texture, colorBufferDefinition, "");
}
