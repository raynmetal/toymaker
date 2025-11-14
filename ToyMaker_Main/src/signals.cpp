#include "toymaker/engine/signals.hpp"

using namespace ToyMaker;

SignalTracker::SignalTracker() = default;

// let copy constructor just create its own signal list
SignalTracker::SignalTracker(const SignalTracker& other): SignalTracker{} { (void)other; /* prevent unused parameter warnings */ }

// move constructor creates own signal list as well
SignalTracker::SignalTracker(SignalTracker&& other): SignalTracker{} { (void)other; /* prevent unused parameter warnings */}

SignalTracker& SignalTracker::operator=(const SignalTracker& other) {
    // Note: There is no point doing any work in this
    // case, as it is the responsibility of the inheritor
    // to make sure signals and connections are correctly
    // reconstructed. Dead signals and observers will 
    // automatically be cleaned up
    (void)other; // prevent unused parameter warnings
    return *this;
}
SignalTracker& SignalTracker::operator=(SignalTracker&& other) {
    // Note: same as in copy assignment
    (void)other; // prevent unused parameter warnings
    return *this;
}

void SignalTracker::connect(const std::string& theirSignalsName, const std::string& ourObserversName, SignalTracker& other) {
    auto otherSignalIter { other.mSignals.find(theirSignalsName) };
    assert(otherSignalIter != other.mSignals.end() && "No signal of this name found on other's tracker");
    std::shared_ptr<ISignal> otherSignal { otherSignalIter->second.lock() };
    assert(otherSignal && "This signal has expired and is no longer valid");

    auto thisObserverIter { mObservers.find(ourObserversName) };
    assert(thisObserverIter != mObservers.end() && "No observer of this name present on this object");
    std::shared_ptr<ISignalObserver> ourObserver { thisObserverIter->second.lock() };
    assert(ourObserver && "This observer has expired and is no longer valid");

    otherSignal->registerObserver(ourObserver);

    garbageCollection();
}

void SignalTracker::garbageCollection() {
    std::vector<std::string> signalsToErase {};
    std::vector<std::string> observersToErase{};
    for(auto& pair: mSignals) {
        if(pair.second.expired()) { 
            signalsToErase.push_back(pair.first);
        }
    }
    for(auto& pair: mObservers) {
        if(pair.second.expired()) {
            observersToErase.push_back(pair.first);
        }
    }

    for(auto& signal: signalsToErase) {
        mSignals.erase(signal);
    }
    for(auto& observer: observersToErase) {
        mObservers.erase(observer);
    }
}
