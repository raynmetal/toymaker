#include <iostream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <GL/glew.h>
#include <assimp/Importer.hpp>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "toymaker/engine/window_context_manager.hpp"

using namespace ToyMaker;

WindowContext::WindowContext(const nlohmann::json& initialWindowConfiguration) {
    const int setHint{
        SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1")
    };
    const int init{
        SDL_Init(SDL_INIT_VIDEO) 
    };
    const int imgInit {
        IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) >= -1 
    };
    const int ttfInit {
        TTF_Init() >= 0 
    };

    assert(setHint && "Could not enable DPI awareness for this SDL app");
    assert(init  >= 0 && "Could not initialise SDL2 library");
    assert(imgInit && "Could not initialise SDL_image library");
    assert(ttfInit && "Could not initialise SDL_ttf library");

    const std::string& applicationTitle { initialWindowConfiguration.at("application_title") };
    const uint32_t windowWidth { initialWindowConfiguration.at("window_width") };
    const uint32_t windowHeight { initialWindowConfiguration.at("window_height") };

    mpSDLWindow = SDL_CreateWindow(
        applicationTitle.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    assert(mpSDLWindow && "Could not create an SDL window");

    if(mpSDLWindow == nullptr){
        std::cout << "Window could not be created" << std::endl;
    }
    std::cout << "Window successfully created!" << std::endl;

    //Specify that we want a forward compatible OpenGL 3.3 context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8); // creating a stencil buffer

    mpGLContext = SDL_GL_CreateContext(mpSDLWindow);
    if(mpGLContext == nullptr) {
        std::cout << "OpenGL context could not be initialized" << std::endl;
    }

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    glewInit();

    // Set up viewport
    glViewport(0, 0, windowWidth, windowHeight);

    // Apply color correction, converting SRGB values to linear space values
    // when in the shader context
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.f, 0.f, 0.f, 0.f);

    // Disable VSync
    SDL_GL_SetSwapInterval(0);

    // Keep the pointer centered when in mouse relative mode
    SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_CENTER, "1");

    std::cout << "GL Version: " << glGetString(GL_VERSION) << std::endl;

    mpAssetImporter = new Assimp::Importer();
    refreshWindowProperties();
}

WindowContext::~WindowContext() {
    std::cout << "Time to say goodbye" << std::endl;
    SDL_GL_DeleteContext(mpGLContext);
    SDL_DestroyWindow(mpSDLWindow);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    delete mpAssetImporter;
}

void WindowContext::swapBuffers() {
    SDL_GL_SwapWindow(mpSDLWindow);
}

void WindowContext::handleWindowEvent(const SDL_WindowEvent& windowEvent) {
    assert(windowEvent.type == SDL_WINDOWEVENT && "Window context cannot handle non-window related events");
    refreshWindowProperties();
    switch(windowEvent.event) {
        case SDL_WINDOWEVENT_ENTER:
            mSigWindowMouseEntered.emit();
        break;
        case SDL_WINDOWEVENT_LEAVE:
            mSigWindowMouseExited.emit();
        break;
        case SDL_WINDOWEVENT_MINIMIZED:
            mSigWindowMinimized.emit();
        break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            mSigWindowMaximized.emit();
        break;
        case SDL_WINDOWEVENT_RESIZED:
            mSigWindowResized.emit();
        break;
        case SDL_WINDOWEVENT_MOVED:
            mSigWindowMoved.emit();
        break;
        case SDL_WINDOWEVENT_SHOWN:
            mSigWindowShown.emit();
        break;
        case SDL_WINDOWEVENT_HIDDEN:
            mSigWindowHidden.emit();
        break;
        case SDL_WINDOWEVENT_EXPOSED:
            mSigWindowExposed.emit();
        break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            mSigWindowSizeChanged.emit();
        break;
        case SDL_WINDOWEVENT_RESTORED:
            mSigWindowRestored.emit();
        break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            mSigWindowKeyFocusGained.emit();
        break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            mSigWindowKeyFocusLost.emit();
        break;
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            mSigWindowKeyFocusOffered.emit();
        break;
        case SDL_WINDOWEVENT_CLOSE:
            mSigWindowCloseRequested.emit();
        break;
        case SDL_WINDOWEVENT_ICCPROF_CHANGED:
            mSigWindowICCProfileChanged.emit();
        break;
        case SDL_WINDOWEVENT_DISPLAY_CHANGED:
            mSigWindowDisplayChanged.emit();
        break;
        default:
            std::cout << "WindowContext: Unrecognized window event\n";
        break;
    }
}

WindowContext& WindowContext::initialize(const nlohmann::json& initialWindowConfiguration) {
    assert(!s_windowContextManager && "This window has already been initialized");
    s_windowContextManager = std::unique_ptr<WindowContext>(new WindowContext{initialWindowConfiguration});
    return getInstance();
}

WindowContext& WindowContext::getInstance() {
    assert(s_windowContextManager && "The window context manager has not been initialized and so cannot be retrieved");
    return *s_windowContextManager;
}

void WindowContext::clear() {
    s_windowContextManager.reset();
}

void WindowContext::refreshWindowProperties() {
    mCachedWindowFlags = static_cast<SDL_WindowFlags>(SDL_GetWindowFlags(mpSDLWindow));
    mCachedDisplayID = SDL_GetWindowDisplayIndex(mpSDLWindow);
    SDL_GetWindowPosition(mpSDLWindow, reinterpret_cast<int*>(&mCachedWindowPosition.x), reinterpret_cast<int*>(&mCachedWindowPosition.y));
    SDL_GetWindowSizeInPixels(mpSDLWindow, reinterpret_cast<int*>(&mCachedWindowDimensions.x), reinterpret_cast<int*>(&mCachedWindowDimensions.y));
    SDL_GetWindowMinimumSize(mpSDLWindow, reinterpret_cast<int*>(&mCachedWindowMinimumDimensions.x), reinterpret_cast<int*>(&mCachedWindowMinimumDimensions.y));
    SDL_GetWindowMaximumSize(mpSDLWindow, reinterpret_cast<int*>(&mCachedWindowMaximumDimensions.x), reinterpret_cast<int*>(&mCachedWindowMaximumDimensions.y));
    mCachedTitle = SDL_GetWindowTitle(mpSDLWindow);
}

bool WindowContext::isMaximized() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_MAXIMIZED;
}
bool WindowContext::isMinimized() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_MINIMIZED;
}
bool WindowContext::isResizable() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_RESIZABLE;
}
bool WindowContext::isHidden() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_HIDDEN;
}
bool WindowContext::hasKeyFocus() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_INPUT_FOCUS;
}
bool WindowContext::hasCapturedMouse() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_MOUSE_CAPTURE;
}
bool WindowContext::hasMouseFocus() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_MOUSE_FOCUS;
}
bool WindowContext::isBorderless() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_BORDERLESS;
}
bool WindowContext::isFullscreen() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_FULLSCREEN;
}
bool WindowContext::isExclusiveFullscreen() const {
    return 
        mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_FULLSCREEN
        && !(mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_FULLSCREEN_DESKTOP);
}
bool WindowContext::isShown() const {
    return mCachedWindowFlags&SDL_WindowFlags::SDL_WINDOW_SHOWN;
}
int WindowContext::getDisplayID() const {
    return mCachedDisplayID;
}
const glm::ivec2 WindowContext::getPosition() const {
    return mCachedWindowPosition;
}
const glm::uvec2 WindowContext::getDimensions() const {
    return mCachedWindowDimensions;
}
const glm::uvec2 WindowContext::getDimensionsMinimum() const {
    return mCachedWindowMinimumDimensions;
}
const glm::uvec2 WindowContext::getDimensionsMaximum() const {
    return mCachedWindowMaximumDimensions;
}
const std::string& WindowContext::getTitle() const {
    return mCachedTitle;
}

void WindowContext::setPosition(const glm::uvec2& newPosition) {
    SDL_SetWindowPosition(mpSDLWindow, newPosition.x, newPosition.y);
    refreshWindowProperties();
}
void WindowContext::setDimensions(const glm::uvec2& newDimensions) {
    SDL_SetWindowSize(mpSDLWindow, newDimensions.x, newDimensions.y);
    refreshWindowProperties();
}
void WindowContext::setResizeAllowed(bool allowed) {
    SDL_SetWindowResizable(mpSDLWindow, allowed? SDL_TRUE: SDL_FALSE);
    refreshWindowProperties();
}
void WindowContext::setDimensionsMinimum(const glm::uvec2& newMinimum) {
    SDL_SetWindowMinimumSize(mpSDLWindow, newMinimum.x, newMinimum.y);
    refreshWindowProperties();
}
void WindowContext::setDimensionsMaximum(const glm::uvec2& newMaximum) {
    SDL_SetWindowMaximumSize(mpSDLWindow, newMaximum.x, newMaximum.y);
    refreshWindowProperties();
}
void WindowContext::maximize() {
    SDL_MaximizeWindow(mpSDLWindow);
    refreshWindowProperties();
}
void WindowContext::minimize() {
    SDL_MinimizeWindow(mpSDLWindow);
    refreshWindowProperties();
}
void WindowContext::restore() {
    SDL_RestoreWindow(mpSDLWindow);
    refreshWindowProperties();
}
void WindowContext::setBorder(bool state) {
    SDL_SetWindowBordered(mpSDLWindow, state? SDL_TRUE: SDL_FALSE);
    refreshWindowProperties();
}
void WindowContext::setTitle(const std::string& newTitle) {
    SDL_SetWindowTitle(mpSDLWindow, newTitle.c_str());
    refreshWindowProperties();
}
void WindowContext::setHidden(bool hide) {
    if(hide && isShown()) SDL_HideWindow(mpSDLWindow);

    else if(!hide && isHidden()) SDL_ShowWindow(mpSDLWindow);

    else return; // no update, so no change in window state

    refreshWindowProperties();
}
void WindowContext::setFullscreen(bool fullscreen, bool exclusive) {
    if(!fullscreen) {
        SDL_SetWindowFullscreen(mpSDLWindow, 0);
    } else {
        SDL_SetWindowFullscreen(mpSDLWindow, exclusive? SDL_WINDOW_FULLSCREEN: SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
    refreshWindowProperties();
}

SDL_GLContext WindowContext::getGLContext() const {
    return mpGLContext;
}

SDL_Window* WindowContext::getSDLWindow() const {
    return mpSDLWindow;
}

Assimp::Importer* WindowContext::getAssetImporter() const {
    return mpAssetImporter;
}
