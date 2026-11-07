// Bounded audio buffer between platform audio sources and the render thread.
//
// Capture callbacks are real-time work, so they only copy samples in here. The
// render thread drains the buffer once per frame and passes the samples to
// projectM. This keeps capture independent of the renderer's lifetime.
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace EyeTune {
namespace Core {

// Implemented by anything that accepts audio from a platform source.
class IAudioSink
{
public:
    virtual ~IAudioSink() = default;

    // Accepts interleaved 16-bit samples with any channel count. Must not block.
    // Returns the number of frames stored.
    virtual std::size_t Write(const std::int16_t* samples, std::size_t frames, unsigned int channels) = 0;
};

// Single-producer, single-consumer ring of stereo frames. Write() runs on the
// capture thread and Read() on the render thread, without locks. When full, new
// frames are dropped and counted, because blocking would stall the audio engine.
class AudioRingBuffer final : public IAudioSink
{
public:
    explicit AudioRingBuffer(std::size_t capacityFrames);

    std::size_t Write(const std::int16_t* samples, std::size_t frames, unsigned int channels) override;

    // Copies up to maxFrames stereo frames to out and returns the frame count.
    std::size_t Read(std::int16_t* out, std::size_t maxFrames);

    std::size_t Capacity() const { return m_capacity; }
    std::size_t AvailableFrames() const;
    std::uint64_t DroppedFrames() const { return m_dropped.load(std::memory_order_relaxed); }

private:
    std::vector<std::int16_t> m_samples;
    std::size_t m_capacity;

    // Count of frames ever written and read. Their difference is the fill level.
    std::atomic<std::uint64_t> m_writeCount{0};
    std::atomic<std::uint64_t> m_readCount{0};
    std::atomic<std::uint64_t> m_dropped{0};
};

} // namespace Core
} // namespace EyeTune
