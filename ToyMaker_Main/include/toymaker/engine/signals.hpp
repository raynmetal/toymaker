/**
 * @ingroup ToyMakerSignals
 * @file signals.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Classes relating to this engine's implementation of signals.  Contains template classes used to define signal senders, receivers, and trackers.
 * @version 0.3.2
 * @date 2025-09-07
 * 
 * 
 */

/**
 * @defgroup ToyMakerSignals Signals
 * @ingroup ToyMakerCore
 * 
 */

#ifndef FOOLSENGINE_SIGNALS_H
#define FOOLSENGINE_SIGNALS_H

#include <memory>
#include <cassert>
#include <functional>
#include <string>
#include <map>
#include <set>

namespace ToyMaker {
    class SignalTracker;
    class ISignalObserver;
    class ISignal;
    template <typename ...TArgs>
    class SignalObserver_;
    template <typename ...TArgs>
    class Signal_;
    template <typename ...TArgs>
    class SignalObserver;
    template <typename ...TArgs>
    class Signal;

    class ISignalObserver {};

    /**
     * @ingroup ToyMakerSignals
     * @brief The base class for any signal that an object advertises.
     * 
     */
    class ISignal {
    public:
        /**
         * @brief A method used by observers to declare their interest in this signal.
         * 
         * @param observer An object interested in this signal.
         */
        virtual void registerObserver(std::weak_ptr<ISignalObserver> observer)=0;
    };

    /**
     * @ingroup ToyMakerSignals
     * @brief A class containing most of the implementation of this engine's Signal concept.
     * 
     * @tparam TArgs The types of data that are sent when this signal is emitted.
     */
    template <typename ...TArgs>
    class Signal_: public ISignal {
    public:
        /**
         * @brief Method via which the owner of the signal can send data to the signal's obervers.
         * 
         * @param args A list of values relating to the signal, to be used by its handlers.
         */
        void emit (TArgs... args);

        /**
         * @brief Causes an eligible observer to be subscribed to this signal.
         * 
         * @param observer An observer interested in this signal.
         */
        void registerObserver(std::weak_ptr<ISignalObserver> observer) override;

    private:
        /**
         * @brief Constructs a new Signal_ object.
         * 
         * This constructor is private, and meant to be accessed only by SignalTracker.
         * 
         */
        Signal_() = default;

        /**
         * @brief Weak references to all the observers that have registered themselves with this Signal.
         * 
         */
        std::set<
            std::weak_ptr<SignalObserver_<TArgs...>>,
            std::owner_less<std::weak_ptr<SignalObserver_<TArgs...>>>
        > mObservers {};

    friend class SignalTracker;
    friend class Signal<TArgs...>;
    };

    /**
     * @ingroup ToyMakerSignals
     * @brief A class containing most of the implementation for this engine's SignalObserver concept.
     * 
     * A signal observer may subscribe to any Signal that sends the same data the observer declares it is capable of handling.
     * 
     * @tparam TArgs A list of types of data whose values the observer will receive when a Signal is emitted.
     */
    template <typename ...TArgs>
    class SignalObserver_: public ISignalObserver {
    public:
        /**
         * @brief The function called by a Signal this oberver is subscribed to.
         * 
         * @param args Arguments populated by the Signal emitted.
         */
        void operator() (TArgs... args);
    private:
        /**
         * @brief Constructs a new Signal Observer_ object
         * 
         * @param callback The function called by the observer when it receives a Signal.
         */
        SignalObserver_(std::function<void(TArgs...)> callback);

        /**
         * @brief A reference to the function stored by this observer.
         * 
         */
        std::function<void(TArgs...)> mStoredFunction {};
    friend class SignalTracker;
    friend class SignalObserver<TArgs...>;
    };

    /**
     * @ingroup ToyMakerSignals
     * @brief A signal tracker, the main interface between an object and the signal system.
     * 
     * Connections in JSON scene descriptions may appear as follows:
     * 
     * ```jsonc
     * 
     * {
     *     "from": "/viewport_UI/return/@UIButton",
     *     "signal": "ButtonReleased",
     * 
     *     "to": "/@UrUINavigation",
     *     "observer": "ButtonClickedObserved"
     * }
     * 
     * ```
     * 
     * @remarks SceneNodeCores have a signal tracker member, allowing them to interface with the scene system.
     * 
     * @see Signal
     * @see SignalObserver
     * 
     */
    class SignalTracker {
    public:
        /**
         * @brief Constructs a new SignalTracker object.
         * 
         */
        SignalTracker();

        /**
         * @brief Constructs a new SignalTracker.
         * 
         * @param other The object being copied from.
         */
        SignalTracker(const SignalTracker& other);
    
        /**
         * @brief Copy assignment operator.
         * 
         * @param other The signal tracker being copied from.
         * @return SignalTracker& The new SignalTracker.
         */
        SignalTracker& operator=(const SignalTracker& other);

        /**
         * @brief Moves resources from another SignalTracker into this one, invalidating them from the other.
         * 
         * @param other The tracker whose resources are being claimed.
         */
        SignalTracker(SignalTracker&& other);

        /**
         * @brief Moves resources from another SignalTracker into this one, destroying this tracker's resources in the process.
         * 
         * @param other The tracker whose resources are being stolen.
         * @return SignalTracker& A reference to this tracker with its properties updated.
         * 
         */
        SignalTracker& operator=(SignalTracker&& other);

        /**
         * @brief Method that connects one of this objects SignalObservers to another tracker's Signal
         * 
         * @param theirSignal The Signal a SignalObserver for this object is being connected to.
         * @param ourObserver Our SignalObserver.
         * @param other The SignalTracker owning the Signal our SignalObserver is trying to subscribe to.
         */
        void connect(const std::string& theirSignal, const std::string& ourObserver, SignalTracker& other);

    private:

        /**
         * @brief Declares a Signal owned by this tracker, and returns a reference to it.
         * 
         * @tparam TArgs A list of types of data sent by this object when the declared signal is emitted.
         * 
         * @param signalName The name of the signal being declared.
         * 
         * @return std::shared_ptr<Signal_<TArgs...>> A reference to the newly constructed signal.
         * 
         */
        template <typename ...TArgs>
        std::shared_ptr<Signal_<TArgs...>> declareSignal(
            const std::string& signalName
        );

        /**
         * @brief Declares a SignalObserver owned by this tracker, returns a reference to it.
         * 
         * @tparam TArgs A list of types of data received by this object when a Signal it is subscribed to is emitted.
         * @param observerName The name of the SignalObserver owned by this object.
         * @param callbackFunction A reference to the function to be called by this observer when a Signal is received by it.
         * @return std::shared_ptr<SignalObserver_<TArgs...>> A reference to the newly constructed SignalObserver.
         */
        template <typename ...TArgs>
        std::shared_ptr<SignalObserver_<TArgs...>> declareSignalObserver(
            const std::string& observerName,
            std::function<void(TArgs...)> callbackFunction 
        );

        /**
         * @brief A method which removes any signals and observers sitting on this object which were destroyed at some point.
         * 
         */
        void garbageCollection();

        /**
         * @brief A list of weak references to this object's SignalObservers, along with their names.
         * 
         */
        std::unordered_map<std::string, std::weak_ptr<ISignalObserver>> mObservers {};

        /**
         * @brief A list of weak references to this object's Signals, along with their names.
         * 
         */
        std::unordered_map<std::string, std::weak_ptr<ISignal>> mSignals {};
    friend class ISignal;
    friend class IObserver;

    template <typename ...TArgs>
    friend class SignalObserver;

    template <typename ...TArgs>
    friend class Signal;
    };

    /**
     * @ingroup ToyMakerSignals
     * @brief A Signal object, designed to emit signals matching some data signature to be received by all the SignalObservers subscribed to it.
     * 
     * It is essentially a wrapper over Signal_<T>
     * 
     * ## Usage
     * 
     * ```c++
     * 
     * class SomeClass : public ToyMaker::SignalTracker {
     * 
     *     // ... 
     * 
     * public:
     *     ToyMaker::Signal<> mSigViewUpdateStarted { *this, "ViewUpdateStarted" };
     * 
     *     ToyMaker::Signal<GameScoreData> mSigScoreUpdated { *this, "ScoreUpdated" };
     * 
     *     // ...
     * 
     * };
     * 
     * ```
     * 
     * Here we see two signals declared, one that sends no data, and one that sends GameScoreData, named "ViewUpdateStarted" and "ScoreUpdated" respectively.
     * 
     * When a SignalObserver (with the correct signature) wishes to connect to these signals, a function call resembling the one below should be made:
     * 
     * ```c++
     * 
     * signalTo.connect(
     *     "ViewUpdateStarted",
     *     "ObserveViewUpdateStarted",
     *     signalFrom
     * );
     * 
     * ```
     * 
     * Where:
     * 
     *   - `signalTo` is a reference to the SignalTracker (or its subclass) owning the SignalObserver.
     * 
     *   - `signalFrom` is a reference to the SignalTracker (or its subclass) owning the Signal
     * 
     *   - `"ViewUpdateStarted"` is the name of the Signal being subscribed to.
     * 
     *   - `"ObserveViewUpdateStarted"` is the name of the SignalObserver requesting subscription.
     * 
     * 
     * @tparam TArgs A list of data types this Signal is expected to send when a signal is emitted.
     * 
     * @see SignalTracker
     * @see SignalObserver
     */
    template <typename ...TArgs>
    class Signal {
    public:
        /**
         * @brief Constructs a Signal object and associates it with its SignalTracker.
         * 
         * @param owningTracker The tracker responsible for advertising this signal.
         * @param name The name  of this signal.
         */
        Signal(SignalTracker& owningTracker, const std::string& name); 

        Signal(const Signal& other) = delete;
        Signal(Signal&& other) = delete;
        Signal& operator=(const Signal& other) = delete;
        Signal& operator=(Signal&& other) = delete;

        /**
         * @brief A method on the signal which causes the signal to be sent to all of its subscribers (aka SignalObservers).
         * 
         * @param args A list of values sent along with this signal.
         */
        void emit(TArgs...args);

        /**
         * @brief Reinitializes the tracker with a new owning SignalTracker.
         * 
         * Useful for copy and move operations on the object this signal may be a member of.
         * 
         * @param owningTracker A reference to this Signal's new owning tracker.
         * @param name The (possibly new) name of this signal, per its owner.
         * 
         * @todo Investigate the behaviour of this method.
         * 
         */
        void resetSignal(SignalTracker& owningTracker, const std::string& name);

    private:
        /**
         * @brief Registers a compatible SignalObserver as a subscriber of this Signal.
         * 
         * @param observer The observer requesting subscription.
         */
        void registerObserver(SignalObserver<TArgs...>& observer);

        /**
         * @brief The actual object connected with this Signal's signal tracker, hidden from users of Signal.
         * 
         */
        std::shared_ptr<Signal_<TArgs...>> mSignal_;

    friend class SignalObserver<TArgs...>;
    };


    /**
     * @ingroup ToyMakerSignals
     * @brief A SignalObserver object, which can subscribe to Signals matching  its data signature and receive signal events from them.
     * 
     * It is essentially a wrapper over SignalObserver_<T>.
     * 
     * ## Usage:
     * 
     * ```c++
     * 
     * class YourClass: public Toymaker::SignalTracker {
     * 
     *     // ...
     * 
     *     void onViewUpdatesCompleted(const std::string& viewName);
     * 
     * public:
     * 
     *     ToyMaker::SignalObserver<const std::string&> mObserveViewUpdateCompleted {
     *         *this, "ViewUpdateCompletedObserved",
     *         [this](const std::string& viewName) { this->onViewUpdatesCompleted(viewName); }
     *     };
     * 
     *     // ...
     * };
     * 
     * ```
     * 
     * @tparam TArgs A list of types of data this observer expects to receive when it receives a signal event.
     * 
     * @see Signal
     * 
     * @see SignalTracker
     * 
     */
    template <typename ...TArgs>
    class SignalObserver {
    public:
        /**
         * @brief Creates a new SignalObserver object.
         * 
         * @param owningTracker The tracker which owns and advertises this SignalObserver.
         * @param name The name of the SignalObserver.
         * @param callback A reference to the function to be called when this observer receives a signal.
         */
        SignalObserver(SignalTracker& owningTracker, const std::string& name, std::function<void(TArgs...)> callback);

        SignalObserver(const SignalObserver& other)=delete;
        SignalObserver(SignalObserver&& other)=delete;
        SignalObserver& operator=(const SignalObserver& other) = delete;
        SignalObserver& operator=(SignalObserver&& other) = delete;

        /**
         * @brief Reinitializes this observer with a new SignalTracker.
         * 
         * Possibly useful for when a move or copy assignment occurs, since a SignalObserver is neither movable nor copyable.
         * 
         * @param owningTracker The new tracker taking charge of this observer.
         * @param name The (possibly new) name of this observer..
         * @param callback The (possibly new) reference to a function to be called by the observer when it receives a Signal event.
         * 
         * @todo Figure out what the intention for this method was.
         * 
         */
        void resetObserver(SignalTracker& owningTracker, const std::string& name, std::function<void(TArgs...)> callback);

        /**
         * @brief Subscribes this SignalObserver to a Signal whose signature matches its own.
         * 
         * @param signal The Signal this SignalObserver should subscribe to.
         */
        void connectTo(Signal<TArgs...>& signal);
    
    private:

        /**
         * @brief The underlying implementation of the observer template.
         * 
         */
        std::shared_ptr<SignalObserver_<TArgs...>> mSignalObserver_;
    
    friend class Signal<TArgs...>;
    };

    template <typename ...TArgs>
    inline void Signal_<TArgs...>::registerObserver(std::weak_ptr<ISignalObserver> observer) {
        assert(!observer.expired() && "Cannot register a null pointer as an observer");
        mObservers.insert(std::static_pointer_cast<SignalObserver_<TArgs...>>(observer.lock()));
    }
    template <typename ...TArgs>
    void Signal_<TArgs...>::emit (TArgs ... args) {
        //observers that will be removed from the list after this signal has been emitted
        std::vector<std::weak_ptr<SignalObserver_<TArgs...>>> expiredObservers {};

        for(auto observer: mObservers) {
            // lock means that this observer is still active
            if(std::shared_ptr<SignalObserver_<TArgs...>> activeObserver = observer.lock()) {
                (*activeObserver)(args...);

            // go to the purge list
            } else {
                expiredObservers.push_back(observer);
            }
        }

        // remove dead observers
        for(auto expiredObserver: expiredObservers) {
            mObservers.erase(expiredObserver);
        }
    }

    template <typename ...TArgs>
    inline SignalObserver_<TArgs...>::SignalObserver_(std::function<void(TArgs...)> callback):
    mStoredFunction{ callback }
    {}
    template <typename ...TArgs>
    inline void SignalObserver_<TArgs...>::operator() (TArgs ... args) { 
        mStoredFunction(args...);
    }

    template <typename ...TArgs>
    inline std::shared_ptr<Signal_<TArgs...>> SignalTracker::declareSignal(const std::string& name) {
        std::shared_ptr<ISignal> newSignal { new Signal_<TArgs...>{} };
        mSignals.insert({name, newSignal});
        garbageCollection();
        return std::static_pointer_cast<Signal_<TArgs...>>(newSignal);
    }
    template <typename ...TArgs>
    inline std::shared_ptr<SignalObserver_<TArgs...>> SignalTracker::declareSignalObserver(const std::string& name, std::function<void(TArgs...)> callback) {
        std::shared_ptr<ISignalObserver> newObserver { new SignalObserver_<TArgs...>{callback} };
        mObservers.insert({name, newObserver});
        garbageCollection();
        return std::static_pointer_cast<SignalObserver_<TArgs...>>(newObserver);
    }
    template <typename ...TArgs>
    Signal<TArgs...>::Signal(SignalTracker& owningTracker, const std::string& name) {
        resetSignal(owningTracker, name);
    }
    template <typename ...TArgs>
    void Signal<TArgs...>::emit(TArgs...args) {
        mSignal_->emit(args...);
    }
    template <typename ...TArgs>
    void Signal<TArgs...>::resetSignal(SignalTracker& owningTracker, const std::string& name) {
        mSignal_ = owningTracker.declareSignal<TArgs...>(name);
    }
    template <typename ...TArgs>
    void Signal<TArgs...>::registerObserver(SignalObserver<TArgs...>& observer) {
        mSignal_->registerObserver(observer.mSignalObserver_);
    }

    template <typename ...TArgs>
    SignalObserver<TArgs...>::SignalObserver(SignalTracker& owningTracker, const std::string& name, std::function<void(TArgs...)> callback) {
        resetObserver(owningTracker, name, callback);
    };
    template <typename ...TArgs>
    void SignalObserver<TArgs...>::resetObserver(SignalTracker& owningTracker, const std::string& name, std::function<void(TArgs...)> callback) {
        assert(callback && "Empty callback is not allowed");
        mSignalObserver_ = owningTracker.declareSignalObserver<TArgs...>(name, callback);
    }
    template <typename ...TArgs>
    void SignalObserver<TArgs...>::connectTo(Signal<TArgs...>& signal) {
        signal.registerObserver(*this);
    }
}

#endif
