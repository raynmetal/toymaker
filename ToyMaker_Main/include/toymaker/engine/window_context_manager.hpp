/**
 * @ingroup ToyMakerEngine
 * @file window_context_manager.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains classes and functions for managing the (at present, single) window of this application.
 * @version 0.3.2
 * @date 2025-09-11
 * 
 * 
 */

#ifndef FOOLSENGINE_WINDOWCONTEXT_H
#define FOOLSENGINE_WINDOWCONTEXT_H

#include <memory>

#include <SDL3/SDL.h>
#include <GL/glew.h>

#include <assimp/Importer.hpp>

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include "signals.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerEngine
     * @brief A class providing access to various window management methods and window event Signals.
     * 
     * It is essentially a convenient wrapper over SDL_Window.
     * 
     */
    class WindowContext {
        /**
         * @brief The signal tracker responsible for publishing window related signals and tracking their observers.
         * 
         */
        SignalTracker mSignalTracker {};
    public:
        /* Deletes SDL and GL contexts */
        ~WindowContext();

        /**
         * @brief Gets this application's sole window instance.
         * 
         * @return WindowContext& This application's window.
         */
        static WindowContext& getInstance();

        WindowContext() = delete;

        /** 
         * @brief Accessor for the OpenGL context pointer
         */
        SDL_GLContext getGLContext() const;
        /** 
         * @brief Accessor for the SDL Window pointer
         */
        SDL_Window* getSDLWindow() const;
        /** 
         * @brief Accessor for asset importer (which for some reason is tied to this WindowContext)
         */
        Assimp::Importer* getAssetImporter() const;

        /**
         * @brief Converts an SDL window event into its corresponding engine Signal equivalent, which it then broadcasts.
         * 
         * @param windowEvent The SDL window event to be converted.
         */
        void handleWindowEvent(const SDL_WindowEvent& windowEvent);

        /**
         * @brief Initializes this window context with the settings specified in this project's `project.json` file.
         * 
         * @param initialWindowConfiguration The window configuration for this application.
         * @return WindowContext& The created window context.
         */
        static WindowContext& initialize(const nlohmann::json& initialWindowConfiguration);

        /**
         * @brief Loses reference to the singleton window context, initiating its destruction.
         * 
         * Usually called as part of the application cleanup process.
         * 
         */
        static void clear();

        /**
         * @brief Swaps the back and front buffers of the framebuffer associated with the application window.
         * 
         */
        void swapBuffers();

        /**
         * @brief Gets the title of this window.
         * 
         * @return const std::string& This window's title.
         * 
         * @todo This shouldn't be returning a const reference.
         */
        const std::string& getTitle() const;

        /**
         * @brief Tests whether this window is maximized.
         * 
         * @retval true This window is maximized.
         * @retval false This window is not maximized.
         */
        bool isMaximized() const;

        /**
         * @brief Tests whether this window is minimized.
         * 
         * @retval true This window is minimized.
         * @retval false This window is not minimized.
         */
        bool isMinimized() const;

        /**
         * @brief Tests whether this window is resizable.
         * 
         * @retval true This window is resizable.
         * @retval false This window is unresizable.
         */
        bool isResizable() const;

        /**
         * @brief Tests whether this window is hidden (as in, there is another window on top of it, or it is minimized (I think)).
         * 
         * @retval true This window is hidden.
         * @retval false This window is not hidden.
         */
        bool isHidden() const;

        /**
         * @brief Tests whether this window is shown, inverse of hidden.
         * 
         * @retval true This window is shown.
         * @retval false This window is not shown.
         * 
         * @see isHidden()
         */
        bool isShown() const;

        /**
         * @brief Tests whether this window has keyboard focus.
         * 
         * @retval true This window has keyboard focus.
         * @retval false This window does not have keyboard focus.
         */
        bool hasKeyFocus() const;

        /**
         * @brief Tests whether this window has mouse focus.
         * 
         * @retval true This window has mouse focus.
         * @retval false This window does not have mouse focus.
         */
        bool hasMouseFocus() const;

        /**
         * @brief Tests whether this window has captured the mouse.
         * 
         * @retval true This window has captured the mouse.
         * @retval false This window has not captured the mouse.
         */
        bool hasCapturedMouse() const;

        /**
         * @brief Tests whether this window is in fullscreen.
         * 
         * @retval true This window is in full screen.
         * @retval false This window is not in full screen.
         */
        bool isFullscreen() const;

        /**
         * @brief Tests whether a fullscreen window is using exclusive fullscreen.
         * 
         * @retval true This window is using exclusive fullscreen.
         * @retval false This window is using borderless window fullscreen or isn't in fullscreen at all.
         */
        bool isExclusiveFullscreen() const;

        /**
         * @brief Tests whether this window is using windowed borderless fullscreen.
         * 
         * @retval true This window is using windowed borderless fullscreen.
         * @retval false This window is using exclusive fullscreen, or isn't in fullscreen at all.
         */
        bool isBorderless() const;

        /**
         * @brief Gets the ID associated with the monitor this window is primarily being displayed on.
         * 
         * @return int The ID of the monitor this window is being displayed on.
         */
        int getDisplayID() const;

        /**
         * @brief Gets the position of this window (relative to the top left corner of the screen) in screen coordinates.
         * 
         * @return const glm::ivec2 The position of this window.
         * 
         * @todo Compute and cache the window position in pixels instead.
         */
        const glm::ivec2 getPosition() const;

        /**
         * @brief Gets the dimensions of this window (in pixels).
         * 
         * @return const glm::uvec2 The dimensions of this window.
         */
        const glm::uvec2 getDimensions() const;

        /**
         * @brief Gets the minimum dimensions allowed for this window (in screen coordinates).
         * 
         * @return const glm::uvec2 The minimum dimensions allowed for this window (in screen coordinates).
         * 
         * @todo Compute and cache minimum window dimensions in pixels instead.
         */
        const glm::uvec2 getDimensionsMinimum() const;
        /**
         * @brief Gets the maximum dimensions allowed for this window (in screen coordinates).
         * 
         * @return const glm::uvec2 The maximum dimensions allowed for this window (in screen coordinates).
         * 
         * @todo Compute and cache minimum window dimensions in pixels instead.
         */
        const glm::uvec2 getDimensionsMaximum() const;

        /**
         * @brief Sets the position of this window relative to the top left corner of the screen, in screen coordinates.
         * 
         * @param position The new position of this window.
         * 
         * @todo Have position be specified in pixels instead.
         */
        void setPosition(const glm::uvec2& position);

        /**
         * @brief Sets the width and height of the window, in pixels (since SDL_WINDOW_ALLOW_HIGHDPI ought to have been called).
         * 
         * @param dimensions The new dimensions for this window.
         */
        void setDimensions(const glm::uvec2& dimensions);

        /**
         * @brief Enables or disables the resizing of this window.
         * 
         * @param allowed Whether this window should be resizable.
         */
        void setResizeAllowed(bool allowed);

        /**
         * @brief Adds or removes the border around this window.
         * 
         * @param state Whether the border is enabled or disabled.
         */
        void setBorder(bool state);

        /**
         * @brief Sets this window's visibility.
         * 
         * @param hide Whether to hide or show this window.
         */
        void setHidden(bool hide);

        /**
         * @brief Sets the title for this window.
         * 
         * @param newTitle This window's new title.
         */
        void setTitle(const std::string& newTitle);

        /**
         * @brief Sets the minimum allowable dimensions for this window in screen coordinates.
         * 
         * @param dimensions This window's new minimum allowable dimensions.
         */
        void setDimensionsMinimum(const glm::uvec2& dimensions);

        /**
         * @brief Sets the maximum allowable dimensions for this window, in screen coordinates.
         * 
         * @param dimensions This window's new maximum allowable dimensions.
         */
        void setDimensionsMaximum(const glm::uvec2& dimensions);

        /**
         * @brief Makes this window fullscreen.
         * 
         * @param fullscreen Whether or not to enable fullscreen.
         */
        void setFullscreen(bool fullscreen);

        /**
         * @brief Maximizes this window.
         * 
         */
        void maximize();

        /**
         * @brief Minimizes this window.
         * 
         */
        void minimize();

        /**
         * @brief Restores this window (if it has been minimized).
         * 
         */
        void restore();

        /**
         * @brief Signal emitted when this window's dimensions are changed.
         * 
         */
        Signal<> mSigWindowResized { mSignalTracker, "WindowResized" };

        /**
         * @brief Signal emitted when this window is maximized.
         * 
         */
        Signal<> mSigWindowMaximized { mSignalTracker, "WindowMaximized" };

        /**
         * @brief Signal emitted when this window is minimized.
         * 
         */
        Signal<> mSigWindowMinimized { mSignalTracker, "WindowMinimized" };

        /**
         * @brief Signal emitted when this window is repositioned.
         * 
         */
        Signal<> mSigWindowMoved { mSignalTracker, "WindowMoved" };

        /**
         * @brief Signal emitted when the mouse has entered this window.
         * 
         */
        Signal<> mSigWindowMouseEntered { mSignalTracker, "WindowMouseEntered" };

        /**
         * @brief Signal emitted when the mouse leaves this window.
         * 
         */
        Signal<> mSigWindowMouseExited { mSignalTracker, "WindowMouseExited" };

        /**
         * @brief Signal emitted when this window is shown.
         * 
         */
        Signal<> mSigWindowShown { mSignalTracker, "WindowShown" };

        /**
         * @brief Signal emitted when this window is hidden.
         * 
         */
        Signal<> mSigWindowHidden { mSignalTracker, "WindowHidden" };

        /**
         * @brief Signal emitted when this window is exposed.
         * 
         */
        Signal<> mSigWindowExposed { mSignalTracker, "WindowExposed" };

        /**
         * @brief Signal emitted when this window is resized.
         * 
         * @todo Figure out how this is related with mSigWindowResized()
         * 
         */
        Signal<> mSigWindowSizeChanged { mSignalTracker, "WindowSizeChanged" };

        /**
         * @brief Signal emitted when this window is restored (after being minimized).
         * 
         */
        Signal<> mSigWindowRestored { mSignalTracker, "WindowRestored" };

        /**
         * @brief Signal emitted when this window receives keyboard focus.
         * 
         */
        Signal<> mSigWindowKeyFocusGained { mSignalTracker, "WindowKeyFocusGained" };

        /**
         * @brief Signal emitted when this window loses keyboard focus.
         * 
         */
        Signal<> mSigWindowKeyFocusLost { mSignalTracker, "WindowKeyFocusLost" };

        /**
         * @brief Signal emitted when the user attempts to close this window (by the X button in the window border usually).
         * 
         */
        Signal<> mSigWindowCloseRequested { mSignalTracker, "WindowClosed" };

        /**
         * @brief Signal emitted when the application offers keyboard focus to the user.
         * 
         */
        Signal<> mSigWindowKeyFocusOffered { mSignalTracker, "WindowKeyFocusOffered" };

        /**
         * @brief I don't know what this is quite honestly.
         * 
         * @todo Figure out what this means.
         * 
         */
        Signal<> mSigWindowICCProfileChanged { mSignalTracker, "WindowICCProfileChanged" };

        /**
         * @brief Signal emitted when the window display changes. 
         * 
         * @todo Figure out what this means.
         * 
         */
        Signal<> mSigWindowDisplayChanged { mSignalTracker, "WindowDisplayChanged" };

    private:
        /**
         * @brief Replaces currently cached window properties.
         * 
         */
        void refreshWindowProperties();

        /**
         * @brief Initializes SDL and OpenGL contexts, creates an SDL window 
        and stores a reference to it.
         */
        WindowContext(const nlohmann::json& initialWindowConfiguration);

        // non copyable
        WindowContext(const WindowContext& other) = delete;
        // non moveable
        WindowContext(WindowContext&& other) = delete;
        // non copy-assignable
        WindowContext& operator=(const WindowContext& other) = delete;
        // non move-assignable
        WindowContext& operator=(WindowContext&& other) = delete;

        /**
         * @brief The SDL window handle this class is a wrapper over.
         * 
         */
        SDL_Window* mpSDLWindow;

        /**
         * @brief The OpenGL context associated with this window.
         * 
         * 
         * @todo move this out of the window context manager, or move window management itself into a class with a smaller scope.
         */
        SDL_GLContext mpGLContext;

        /**
         * @brief The asset importer asset associated with this window (and therefore the whole project)
         * 
         * @todo move this out of the window context manager, or move window management itself into a class with a smaller scope.
         * 
         */
        Assimp::Importer* mpAssetImporter;

        /**
         * @brief A number whose bits represent different modes a window can be in.
         * 
         */
        SDL_WindowFlags mCachedWindowFlags {};

        /**
         * @brief Immutable pointer to the current display mode for this window, whose data is managed by
         * SDL
         * 
         */
        SDL_DisplayMode* const mDisplayMode {};

        /**
         * @brief The cached position of this window, in screen coordinates.
         * 
         * @todo Make it so that this is stored in pixels instead, somehow.
         */
        glm::i16vec2 mCachedWindowPosition {};

        /**
         * @brief The cached dimensions of this window, in pixels.
         * 
         */
        glm::u16vec2 mCachedWindowDimensions {};

        /**
         * @brief The minimum allowed dimensions for this window, in screen coordinates.
         * 
         * @todo Make it so that this is stored in pixels instead, somehow.
         * 
         */
        glm::u16vec2 mCachedWindowMinimumDimensions {};

        /**
         * @brief The maximum allowed dimensions for this window, in screen coordinates.
         * 
         * @todo Make it so that this is stored in pixels instead, somehow.
         * 
         */
        glm::u16vec2 mCachedWindowMaximumDimensions {};

        /**
         * @brief The ID of the display (or monitor) this window is rendered on.
         * 
         */
        int mCachedDisplayID {0};

        /**
         * @brief The cached title of this window.
         * 
         */
        std::string mCachedTitle {};

        /**
         * @brief A pointer to the single static instance of the WindowContext associated with the application.
         * 
         * @todo Eventually support multi-window applications.
         */
        inline static std::unique_ptr<WindowContext> s_windowContextManager { nullptr };
    };
}

#endif
