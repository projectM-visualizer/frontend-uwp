// The only code in projectM-UWP that calls projectM. It wraps the C and playlist APIs,
// so the rest of the app doesn't depend on their handles or memory rules.
#pragma once

#include "AudioRingBuffer.h"
#include "FrameStats.h"
#include "Settings.h"

#include <projectM-4/playlist.h>
#include <projectM-4/projectM.h>

#include <cstddef>
#include <string>
#include <vector>

namespace projectM_UWP {

class Visualizer
{
public:
    Visualizer() = default;
    ~Visualizer();

    Visualizer(const Visualizer&) = delete;
    Visualizer& operator=(const Visualizer&) = delete;

    // Needs a current OpenGL ES 3 context. Returns false if projectM can't start.
    bool Create(projectm_load_proc loadProc, void* userData);

    // Must run while the context that Create used is still current.
    void Destroy();

    bool IsCreated() const { return m_projectm != nullptr; }

    void SetViewport(int width, int height);
    void ApplySettings(const Core::Settings& settings);

    // Replaces the playlist with the presets in these folders. With favoritesOnly
    // set, only favorites are kept, unless none of them are present.
    void LoadPresets(const std::vector<std::string>& folders, const Core::Settings& settings);

    // Passes queued audio to projectM, then draws one frame.
    void RenderFrame(Core::AudioRingBuffer& audio, Core::FrameStats& stats);

    void NextPreset();
    void PreviousPreset();
    void RandomPreset();

    bool TogglePresetLock();
    bool IsPresetLocked() const;

    std::size_t PresetCount() const;
    std::string CurrentPresetPath() const; // Empty when the playlist is empty.

private:
    projectm_handle m_projectm = nullptr;
    projectm_playlist_handle m_playlist = nullptr;
    std::vector<std::int16_t> m_audio;
};

} // namespace projectM_UWP
