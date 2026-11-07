#include "Settings.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace EyeTune {
namespace Core {

namespace {

constexpr int MaxSoftCutSeconds = 60;

// Reads a whole number. Returns false for anything else.
bool ParseInt(const std::string& text, int& value)
{
    if (text.empty())
    {
        return false;
    }

    char* end = nullptr;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (*end != '\0' || parsed < -1000000 || parsed > 1000000)
    {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

bool ParseBool(const std::string& text, bool& value)
{
    if (text == "1" || text == "true")
    {
        value = true;
        return true;
    }
    if (text == "0" || text == "false")
    {
        value = false;
        return true;
    }
    return false;
}

int Clamp(int value, int low, int high)
{
    return std::max(low, std::min(value, high));
}

int HexValue(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

std::string DecodeEscapes(const std::string& text)
{
    std::string decoded;
    decoded.reserve(text.size());

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '%' && i + 2 < text.size())
        {
            const int high = HexValue(text[i + 1]);
            const int low = HexValue(text[i + 2]);
            if (high >= 0 && low >= 0)
            {
                decoded.push_back(static_cast<char>(high * 16 + low));
                i += 2;
                continue;
            }
        }
        decoded.push_back(text[i]);
    }

    return decoded;
}

} // namespace

Settings Settings::Parse(const std::string& text)
{
    Settings settings;
    std::istringstream lines(text);
    std::string line;

    while (std::getline(lines, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        const std::size_t separator = line.find('=');
        if (separator == std::string::npos)
        {
            continue;
        }

        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        int number = 0;

        if (key == "presetDurationSeconds" && ParseInt(value, number))
        {
            settings.presetDurationSeconds = Clamp(number, MinPresetDuration, MaxPresetDuration);
        }
        else if (key == "softCutSeconds" && ParseInt(value, number))
        {
            settings.softCutSeconds = Clamp(number, 0, MaxSoftCutSeconds);
        }
        else if (key == "shuffle")
        {
            ParseBool(value, settings.shuffle);
        }
        else if (key == "favoritesOnly")
        {
            ParseBool(value, settings.favoritesOnly);
        }
        else if (key == "audioSource")
        {
            if (value == "system")
            {
                settings.audioSource = AudioSourceKind::SystemAudio;
            }
            else if (value == "microphone")
            {
                settings.audioSource = AudioSourceKind::Microphone;
            }
        }
        else if (key == "favorite" && !value.empty())
        {
            const std::string favorite = FileNameFromPath(value);
            if (!settings.IsFavorite(favorite))
            {
                settings.favorites.push_back(favorite);
            }
        }
    }

    return settings;
}

std::string Settings::Serialize() const
{
    std::ostringstream out;
    out << "presetDurationSeconds=" << presetDurationSeconds << '\n';
    out << "softCutSeconds=" << softCutSeconds << '\n';
    out << "shuffle=" << (shuffle ? 1 : 0) << '\n';
    out << "favoritesOnly=" << (favoritesOnly ? 1 : 0) << '\n';
    out << "audioSource=" << (audioSource == AudioSourceKind::Microphone ? "microphone" : "system") << '\n';
    for (const std::string& favorite : favorites)
    {
        out << "favorite=" << favorite << '\n';
    }
    return out.str();
}

bool Settings::IsFavorite(const std::string& presetName) const
{
    return std::find(favorites.begin(), favorites.end(), presetName) != favorites.end();
}

bool Settings::ToggleFavorite(const std::string& presetName)
{
    const auto found = std::find(favorites.begin(), favorites.end(), presetName);
    if (found != favorites.end())
    {
        favorites.erase(found);
        return false;
    }

    favorites.push_back(presetName);
    return true;
}

void Settings::AdjustPresetDuration(int deltaSeconds)
{
    presetDurationSeconds = Clamp(presetDurationSeconds + deltaSeconds, MinPresetDuration, MaxPresetDuration);
}

std::string FileNameFromPath(const std::string& path)
{
    const std::size_t slash = path.find_last_of("/\\");
    return DecodeEscapes(slash == std::string::npos ? path : path.substr(slash + 1));
}

} // namespace Core
} // namespace EyeTune
