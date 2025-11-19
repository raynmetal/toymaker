/**
 * @file render_debug_viewer.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains an aspect class used to log window events and change the debug texture rendered to the viewport it manages.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 * 
 */

#ifndef ZOAPPRENDERDEBUGVIEWER_H
#define ZOAPPRENDERDEBUGVIEWER_H

#include "toymaker/engine/sim_system.hpp"
#include "toymaker/engine/window_context_manager.hpp"

namespace ToyMaker {

    /**
     * @brief A utility aspect class used to log window events and change the debug texture rendered to the viewport it manages.
     * 
     */
    class RenderDebugViewer: public ToyMaker::SimObjectAspect<RenderDebugViewer> {
    public:
        inline static std::string getSimObjectAspectTypeName() { return "RenderDebugViewer"; }
        std::shared_ptr<BaseSimObjectAspect> clone() const override;
        static std::shared_ptr<BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);
        ToyMaker::SignalObserver<> mObserveWindowResized { *this, "WindowResizedObserved", [this]() { std::cout << "RenderDebugViewer: Window was resized\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowMinimized { *this, "WindowMinimizedObserved", [this]() { std::cout << "RenderDebugViewer: Window was minimized\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowMaximized { *this, "WindowMaximizedObserved", [this]() { std::cout << "RenderDebugViewer: Window was maximized\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowMoved { *this, "WindowMovedObserved", [this]() { std::cout << "RenderDebugViewer: Window was moved\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowMouseEntered { *this, "WindowMouseEnteredObserved", [this]() { std::cout << "RenderDebugViewer: Mouse entered window\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowMouseExited { *this, "WindowMouseExitedObserved", [this]() { std::cout << "RenderDebugViewer: Mouse left window\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowCloseRequested { *this, "WindowCloseRequestedObserved", [this]() { std::cout << "RenderDebugViewer: Window close requested\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowSizeChanged { *this, "WindowSizeChangedObserved", [this]() { std::cout << "RenderDebugViewer: Window's size was changed\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowRestored { *this, "WindowRestoredObserved", [this]() { std::cout << "RenderDebugViewer: Window was restored\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowShown { *this, "WindowShownObserved", [this]() { std::cout << "RenderDebugViewer: Window was shown\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowExposed { *this, "WindowExposedObserved", [this]() { std::cout << "RenderDebugViewer: Window was exposed\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowKeyFocusGained { *this, "WindowKeyFocusGainedObserved", [this]() { std::cout << "RenderDebugViewer: Window gained key focus\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowKeyFocusLost { *this, "WindowKeyFocusLostObserved", [this]() { std::cout << "RenderDebugViewer: Window lost key focus\n"; this->printWindowProps(); } };
        ToyMaker::SignalObserver<> mObserveWindowKeyFocusOffered { *this, "WindowKeyFocusOffered", [this]() { std::cout << "RenderDebugViewer: Window was offered key focus\n"; this->printWindowProps(); } };
        void printWindowProps();

    protected:
        bool onUpdateGamma(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);
        bool onUpdateExposure(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);
        bool onRenderNextTexture(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);

        std::weak_ptr<ToyMaker::FixedActionBinding> handleUpdateGamma{ declareFixedActionBinding (
            "Graphics", "UpdateGamma", [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                return this->onUpdateGamma(actionData, actionDefinition);
            }
        )};
        std::weak_ptr<ToyMaker::FixedActionBinding> handleUpdateExposure { declareFixedActionBinding (
            "Graphics", "UpdateExposure", [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                return this->onUpdateExposure(actionData, actionDefinition);
            }
        )};
        std::weak_ptr<ToyMaker::FixedActionBinding> handleRenderNextTexture { declareFixedActionBinding(
            "Graphics", "RenderNextTexture", [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                return this->onRenderNextTexture(actionData, actionDefinition);
            }
        )};

    private:

        RenderDebugViewer() : SimObjectAspect<RenderDebugViewer>{0} {
            ToyMaker::WindowContext& windowContextManager { ToyMaker::WindowContext::getInstance() };
            mObserveWindowMoved.connectTo(windowContextManager.mSigWindowMoved);
            mObserveWindowResized.connectTo(windowContextManager.mSigWindowResized);
            mObserveWindowMinimized.connectTo(windowContextManager.mSigWindowMinimized);
            mObserveWindowMaximized.connectTo(windowContextManager.mSigWindowMaximized);
            mObserveWindowMouseEntered.connectTo(windowContextManager.mSigWindowMouseEntered);
            mObserveWindowMouseExited.connectTo(windowContextManager.mSigWindowMouseExited);
            mObserveWindowShown.connectTo(windowContextManager.mSigWindowShown);
            mObserveWindowSizeChanged.connectTo(windowContextManager.mSigWindowSizeChanged);
            mObserveWindowCloseRequested.connectTo(windowContextManager.mSigWindowCloseRequested);
            mObserveWindowRestored.connectTo(windowContextManager.mSigWindowRestored);
            mObserveWindowExposed.connectTo(windowContextManager.mSigWindowExposed);
            mObserveWindowKeyFocusGained.connectTo(windowContextManager.mSigWindowKeyFocusGained);
            mObserveWindowKeyFocusLost.connectTo(windowContextManager.mSigWindowKeyFocusLost);
            mObserveWindowKeyFocusOffered.connectTo(windowContextManager.mSigWindowKeyFocusOffered);
        }

        float mGammaStep { .1f };
        float mExposureStep { .1f };
    };

}

#endif
