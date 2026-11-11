// Joker Mod: global hook for recording the call audio locally.
// The per-call engines call into JokerAudioRecorder from their audio capture/render paths,
// and the app installs a single handler to receive the samples (or clears it when not recording).
// The handler is invoked on the real-time audio thread, so it must be fast (a memcpy at most).
#pragma once

#include <functional>
#include <mutex>
#include <cstdint>
#include <cstddef>

namespace tgcalls {

class JokerAudioRecorder {
public:
    static JokerAudioRecorder &Instance() {
        static JokerAudioRecorder recorder;
        return recorder;
    }

    using Handler = std::function<void(const int16_t *samples, size_t numFrames, uint32_t numChannels, uint32_t samplesPerSec, bool isCapture)>;

    void setHandler(Handler handler) {
        std::lock_guard<std::mutex> lock(mutex_);
        handler_ = std::move(handler);
    }

    void clearHandler() {
        std::lock_guard<std::mutex> lock(mutex_);
        handler_ = nullptr;
    }

    bool isSet() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return handler_ != nullptr;
    }

    void onCapture(const int16_t *samples, size_t numFrames, uint32_t numChannels, uint32_t samplesPerSec) {
        callHandler(samples, numFrames, numChannels, samplesPerSec, true);
    }

    void onRender(const int16_t *samples, size_t numFrames, uint32_t numChannels, uint32_t samplesPerSec) {
        callHandler(samples, numFrames, numChannels, samplesPerSec, false);
    }

private:
    void callHandler(const int16_t *samples, size_t numFrames, uint32_t numChannels, uint32_t samplesPerSec, bool isCapture) {
        Handler handler;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            handler = handler_;
        }
        if (handler) {
            handler(samples, numFrames, numChannels, samplesPerSec, isCapture);
        }
    }

    mutable std::mutex mutex_;
    Handler handler_;
};

}
