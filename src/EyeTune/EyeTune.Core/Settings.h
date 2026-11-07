// User preferences, stored as a small key=value text file in the app's local folder.
#pragma once

#include <string>
#include <vector>

namespace EyeTune {
namespace Core {

enum class AudioSourceKind
{
    SystemAudio, // Loopback of what the default output device is playing.
    Microphone,  // The default capture device.
};

struct Settings
{
    static constexpr int MinPresetDuration = 5;
    static constexpr int MaxPresetDuration = 600;
    static constexpr int PresetDurationStep = 5;

    int presetDurationSeconds = 30;
    int softCutSeconds = 3;
    bool shuffle = true;
    bool favoritesOnly = false;
    AudioSourceKind audioSource = AudioSourceKind::SystemAudio;
    std::vector<std::string> favorites; // Preset file names, without folders.

    // Unknown lines are ignored and bad values keep their defaults, so a damaged or
    // newer file can never stop the app from starting.
    static Settings Parse(const std::string& text);
    std::string Serialize() const;

    bool IsFavorite(const std::string& presetName) const;

    // Returns true if the preset is a favorite after the change.
    bool ToggleFavorite(const std::string& presetName);

    // Applies a change to the preset duration, keeping it within the allowed range.
    void AdjustPresetDuration(int deltaSeconds);
};

// Returns the file name from a path that uses either kind of slash.
std::string FileNameFromPath(const std::string& path);

} // namespace Core
} // namespace EyeTune
