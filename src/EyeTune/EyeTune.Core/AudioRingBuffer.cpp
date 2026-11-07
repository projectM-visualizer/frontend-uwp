#include "AudioRingBuffer.h"

#include <algorithm>

namespace EyeTune {
namespace Core {

AudioRingBuffer::AudioRingBuffer(std::size_t capacityFrames)
    : m_samples(std::max<std::size_t>(capacityFrames, 1) * 2)
    , m_capacity(std::max<std::size_t>(capacityFrames, 1))
{
}

std::size_t AudioRingBuffer::Write(const std::int16_t* samples, std::size_t frames, unsigned int channels)
{
    if (samples == nullptr || frames == 0 || channels == 0)
    {
        return 0;
    }

    const std::uint64_t written = m_writeCount.load(std::memory_order_relaxed);
    const std::uint64_t read = m_readCount.load(std::memory_order_acquire);
    const std::size_t space = m_capacity - static_cast<std::size_t>(written - read);
    const std::size_t count = std::min(frames, space);

    for (std::size_t i = 0; i < count; ++i)
    {
        const std::int16_t* frame = samples + i * channels;
        const std::size_t slot = static_cast<std::size_t>((written + i) % m_capacity) * 2;

        // Mono is copied to both sides. Surround keeps front left and right, which
        // are the first two channels in every Windows channel layout.
        m_samples[slot] = frame[0];
        m_samples[slot + 1] = channels == 1 ? frame[0] : frame[1];
    }

    m_writeCount.store(written + count, std::memory_order_release);

    if (count < frames)
    {
        m_dropped.fetch_add(frames - count, std::memory_order_relaxed);
    }
    return count;
}

std::size_t AudioRingBuffer::Read(std::int16_t* out, std::size_t maxFrames)
{
    if (out == nullptr || maxFrames == 0)
    {
        return 0;
    }

    const std::uint64_t read = m_readCount.load(std::memory_order_relaxed);
    const std::uint64_t written = m_writeCount.load(std::memory_order_acquire);
    const std::size_t count = std::min(maxFrames, static_cast<std::size_t>(written - read));

    for (std::size_t i = 0; i < count; ++i)
    {
        const std::size_t slot = static_cast<std::size_t>((read + i) % m_capacity) * 2;
        out[i * 2] = m_samples[slot];
        out[i * 2 + 1] = m_samples[slot + 1];
    }

    m_readCount.store(read + count, std::memory_order_release);
    return count;
}

std::size_t AudioRingBuffer::AvailableFrames() const
{
    const std::uint64_t written = m_writeCount.load(std::memory_order_acquire);
    const std::uint64_t read = m_readCount.load(std::memory_order_acquire);
    return static_cast<std::size_t>(written - read);
}

} // namespace Core
} // namespace EyeTune
