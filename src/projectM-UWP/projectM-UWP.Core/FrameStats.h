// Measures frame rate and audio level over a reporting interval.
#pragma once

#include <cstddef>
#include <cstdint>

namespace projectM_UWP {
namespace Core {

struct FrameReport
{
    double framesPerSecond = 0.0;
    std::uint64_t audioFrames = 0; // Stereo frames passed to projectM in the interval.
    int audioPeak = 0;             // Largest absolute sample, from 0 to 32768.
};

class FrameStats
{
public:
    explicit FrameStats(double reportIntervalSeconds);

    // Counts one rendered frame.
    void AddFrame();

    // Measures interleaved 16-bit audio about to be passed to projectM.
    void AddAudio(const std::int16_t* samples, std::size_t sampleCount);

    // Returns true and fills report once per interval, then starts a new interval.
    bool Poll(double nowSeconds, FrameReport& report);

private:
    double m_interval;
    double m_intervalStart = -1.0;
    std::uint64_t m_frames = 0;
    std::uint64_t m_audioSamples = 0;
    int m_peak = 0;
};

} // namespace Core
} // namespace projectM_UWP
