#include "FrameStats.h"

#include <algorithm>
#include <cstdlib>

namespace projectM_UWP {
namespace Core {

FrameStats::FrameStats(double reportIntervalSeconds)
    : m_interval(reportIntervalSeconds)
{
}

void FrameStats::AddFrame()
{
    ++m_frames;
}

void FrameStats::AddAudio(const std::int16_t* samples, std::size_t sampleCount)
{
    for (std::size_t i = 0; i < sampleCount; ++i)
    {
        m_peak = std::max(m_peak, std::abs(static_cast<int>(samples[i])));
    }
    m_audioSamples += sampleCount;
}

bool FrameStats::Poll(double nowSeconds, FrameReport& report)
{
    if (m_intervalStart < 0.0)
    {
        m_intervalStart = nowSeconds;
        return false;
    }

    const double elapsed = nowSeconds - m_intervalStart;
    if (elapsed < m_interval)
    {
        return false;
    }

    report.framesPerSecond = static_cast<double>(m_frames) / elapsed;
    report.audioFrames = m_audioSamples / 2;
    report.audioPeak = m_peak;

    m_intervalStart = nowSeconds;
    m_frames = 0;
    m_audioSamples = 0;
    m_peak = 0;
    return true;
}

} // namespace Core
} // namespace projectM_UWP
