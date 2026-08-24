#include "toymaker/engine/sound/system.hpp"

using namespace ToyMaker;

void SoundSystem::onApplicationInitialize() {
    mMixer = std::make_unique<SoundMixer>();
}
void SoundSystem::onApplicationEnd() {
    mMixer = nullptr;
}

std::unique_ptr<SoundChannel> SoundSystem::createChannel() {
    return std::make_unique<SoundChannel>(*mMixer);
}

float SoundSystem::getMasterGain() const {
    return mMixer->getGain();
}

void SoundSystem::setMasterGain(float newVolume) {
    mMixer->setGain(newVolume);
}

