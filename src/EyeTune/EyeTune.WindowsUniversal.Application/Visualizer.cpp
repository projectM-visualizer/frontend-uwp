#include "Visualizer.h"

#include <cstdlib>

namespace EyeTune {

namespace {

const int MeshWidth = 300;
const int MeshHeight = 168; // Matches a 16:9 window.
const int TargetFps = 60;
const float BeatSensitivity = 0.9f;

} // namespace

Visualizer::~Visualizer()
{
    Destroy();
}

bool Visualizer::Create(projectm_load_proc loadProc, void* userData)
{
    Destroy();

    m_projectm = projectm_create_with_opengl_load_proc(loadProc, userData);
    if (m_projectm == nullptr)
    {
        return false;
    }

    m_playlist = projectm_playlist_create(m_projectm);
    if (m_playlist == nullptr)
    {
        Destroy();
        return false;
    }

    projectm_set_mesh_size(m_projectm, MeshWidth, MeshHeight);
    projectm_set_fps(m_projectm, TargetFps);
    projectm_set_beat_sensitivity(m_projectm, BeatSensitivity);
    projectm_set_aspect_correction(m_projectm, true);

    m_audio.resize(static_cast<std::size_t>(projectm_pcm_get_max_samples()) * 2);
    return true;
}

void Visualizer::Destroy()
{
    if (m_playlist != nullptr)
    {
        projectm_playlist_destroy(m_playlist);
        m_playlist = nullptr;
    }
    if (m_projectm != nullptr)
    {
        projectm_destroy(m_projectm);
        m_projectm = nullptr;
    }
}

void Visualizer::SetViewport(int width, int height)
{
    if (m_projectm != nullptr && width > 0 && height > 0)
    {
        projectm_set_window_size(m_projectm, width, height);
    }
}

void Visualizer::ApplySettings(const Core::Settings& settings)
{
    if (m_projectm == nullptr)
    {
        return;
    }

    projectm_set_preset_duration(m_projectm, settings.presetDurationSeconds);
    projectm_set_soft_cut_duration(m_projectm, settings.softCutSeconds);
    projectm_playlist_set_shuffle(m_playlist, settings.shuffle);
}

void Visualizer::LoadPresets(const std::vector<std::string>& folders, const Core::Settings& settings)
{
    if (m_projectm == nullptr)
    {
        return;
    }

    projectm_playlist_clear(m_playlist);

    std::vector<const char*> textureFolders;
    for (const std::string& folder : folders)
    {
        projectm_playlist_add_path(m_playlist, folder.c_str(), true, false);
        textureFolders.push_back(folder.c_str());
    }
    projectm_set_texture_search_paths(m_projectm, textureFolders.data(), textureFolders.size());

    if (settings.favoritesOnly)
    {
        std::vector<std::string> all;
        std::vector<std::string> favorites;

        char** items = projectm_playlist_items(m_playlist, 0, projectm_playlist_size(m_playlist));
        for (char** item = items; item != nullptr && *item != nullptr; ++item)
        {
            all.emplace_back(*item);
            if (settings.IsFavorite(Core::FileNameFromPath(all.back())))
            {
                favorites.push_back(all.back());
            }
        }
        projectm_playlist_free_string_array(items);

        if (!favorites.empty())
        {
            projectm_playlist_clear(m_playlist);
            for (const std::string& favorite : favorites)
            {
                projectm_playlist_add_preset(m_playlist, favorite.c_str(), false);
            }
        }
    }

    NextPreset();
}

void Visualizer::RenderFrame(Core::AudioRingBuffer& audio, Core::FrameStats& stats)
{
    if (m_projectm == nullptr)
    {
        return;
    }

    const std::size_t maxFrames = m_audio.size() / 2;
    std::size_t frames = 0;
    while ((frames = audio.Read(m_audio.data(), maxFrames)) > 0)
    {
        stats.AddAudio(m_audio.data(), frames * 2);
        projectm_pcm_add_int16(m_projectm, m_audio.data(), static_cast<unsigned int>(frames), PROJECTM_STEREO);
    }

    projectm_opengl_render_frame(m_projectm);
}

void Visualizer::NextPreset()
{
    if (PresetCount() > 0)
    {
        projectm_playlist_play_next(m_playlist, true);
    }
}

void Visualizer::PreviousPreset()
{
    if (PresetCount() > 0)
    {
        projectm_playlist_play_previous(m_playlist, true);
    }
}

void Visualizer::RandomPreset()
{
    const std::size_t count = PresetCount();
    if (count > 0)
    {
        projectm_playlist_set_position(m_playlist, static_cast<uint32_t>(std::rand() % count), true);
    }
}

bool Visualizer::TogglePresetLock()
{
    const bool locked = !IsPresetLocked();
    projectm_set_preset_locked(m_projectm, locked);
    return locked;
}

bool Visualizer::IsPresetLocked() const
{
    return m_projectm != nullptr && projectm_get_preset_locked(m_projectm);
}

std::size_t Visualizer::PresetCount() const
{
    return m_playlist != nullptr ? projectm_playlist_size(m_playlist) : 0;
}

std::string Visualizer::CurrentPresetPath() const
{
    if (PresetCount() == 0)
    {
        return std::string();
    }

    std::string path;
    if (char* item = projectm_playlist_item(m_playlist, projectm_playlist_get_position(m_playlist)))
    {
        path = item;
        projectm_playlist_free_string(item);
    }
    return path;
}

} // namespace EyeTune
