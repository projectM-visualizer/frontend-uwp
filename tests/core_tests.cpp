#include "AudioRingBuffer.h"
#include "FrameStats.h"
#include "Settings.h"

#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

using namespace EyeTune::Core;

namespace {

int g_failures = 0;

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition);  \
            ++g_failures;                                                       \
        }                                                                       \
    } while (false)

void TestStereoRoundTrip()
{
    AudioRingBuffer buffer(8);
    const std::int16_t in[] = {1, 2, 3, 4};
    std::int16_t out[8] = {};

    CHECK(buffer.Write(in, 2, 2) == 2);
    CHECK(buffer.AvailableFrames() == 2);
    CHECK(buffer.Read(out, 8) == 2);
    CHECK(out[0] == 1 && out[1] == 2 && out[2] == 3 && out[3] == 4);
    CHECK(buffer.AvailableFrames() == 0);
    CHECK(buffer.Read(out, 8) == 0);
}

void TestMonoIsCopiedToBothChannels()
{
    AudioRingBuffer buffer(4);
    const std::int16_t in[] = {7, 9};
    std::int16_t out[4] = {};

    CHECK(buffer.Write(in, 2, 1) == 2);
    CHECK(buffer.Read(out, 2) == 2);
    CHECK(out[0] == 7 && out[1] == 7 && out[2] == 9 && out[3] == 9);
}

void TestSurroundKeepsFrontChannels()
{
    AudioRingBuffer buffer(4);
    const std::int16_t in[] = {1, 2, 90, 91, 92, 93, 3, 4, 94, 95, 96, 97}; // 5.1
    std::int16_t out[4] = {};

    CHECK(buffer.Write(in, 2, 6) == 2);
    CHECK(buffer.Read(out, 2) == 2);
    CHECK(out[0] == 1 && out[1] == 2 && out[2] == 3 && out[3] == 4);
}

void TestOverflowDropsNewFramesAndCountsThem()
{
    AudioRingBuffer buffer(2);
    const std::int16_t in[] = {1, 1, 2, 2, 3, 3};
    std::int16_t out[6] = {};

    CHECK(buffer.Write(in, 3, 2) == 2);
    CHECK(buffer.DroppedFrames() == 1);
    CHECK(buffer.Read(out, 3) == 2);
    CHECK(out[0] == 1 && out[2] == 2);
}

void TestWrapsAroundTheEnd()
{
    AudioRingBuffer buffer(3);
    std::int16_t out[6] = {};
    const std::int16_t first[] = {1, 1, 2, 2};
    const std::int16_t second[] = {3, 3, 4, 4};

    CHECK(buffer.Write(first, 2, 2) == 2);
    CHECK(buffer.Read(out, 2) == 2);
    CHECK(buffer.Write(second, 2, 2) == 2);
    CHECK(buffer.Read(out, 2) == 2);
    CHECK(out[0] == 3 && out[2] == 4);
}

void TestRejectsInvalidInput()
{
    AudioRingBuffer buffer(2);
    const std::int16_t in[] = {1, 2};
    std::int16_t out[2] = {};

    CHECK(buffer.Write(nullptr, 1, 2) == 0);
    CHECK(buffer.Write(in, 0, 2) == 0);
    CHECK(buffer.Write(in, 1, 0) == 0);
    CHECK(buffer.Read(nullptr, 1) == 0);
    CHECK(buffer.Read(out, 0) == 0);
    CHECK(AudioRingBuffer(0).Capacity() == 1);
}

// Sends a known sequence through the buffer on two threads. Each frame the reader
// gets must be the next one sent, whatever was dropped when the buffer was full.
void TestProducerAndConsumerThreads()
{
    const int frames = 2000000;
    AudioRingBuffer buffer(512);
    bool ordered = true;

    std::thread producer([&] {
        for (int i = 0; i < frames; ++i)
        {
            const std::int16_t frame[] = {static_cast<std::int16_t>(i & 0x7fff),
                                          static_cast<std::int16_t>(~i & 0x7fff)};
            while (buffer.Write(frame, 1, 2) == 0)
            {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&] {
        std::vector<std::int16_t> out(128 * 2);
        int expected = 0;
        while (expected < frames)
        {
            const std::size_t count = buffer.Read(out.data(), 128);
            for (std::size_t i = 0; i < count; ++i, ++expected)
            {
                if (out[i * 2] != (expected & 0x7fff) || out[i * 2 + 1] != (~expected & 0x7fff))
                {
                    ordered = false;
                }
            }
            if (count == 0)
            {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();
    CHECK(ordered);
}

void TestSettingsRoundTrip()
{
    Settings settings;
    settings.presetDurationSeconds = 45;
    settings.softCutSeconds = 7;
    settings.shuffle = false;
    settings.favoritesOnly = true;
    settings.audioSource = AudioSourceKind::Microphone;
    settings.favorites = {"Geiss - Luz.milk", "Name = with equals.milk"};

    const Settings loaded = Settings::Parse(settings.Serialize());

    CHECK(loaded.presetDurationSeconds == 45);
    CHECK(loaded.softCutSeconds == 7);
    CHECK(!loaded.shuffle);
    CHECK(loaded.favoritesOnly);
    CHECK(loaded.audioSource == AudioSourceKind::Microphone);
    CHECK(loaded.favorites == settings.favorites);
}

void TestDamagedSettingsKeepDefaults()
{
    const Settings defaults;
    const Settings loaded = Settings::Parse(
        "garbage line\n"
        "presetDurationSeconds=abc\n"
        "softCutSeconds=\n"
        "shuffle=maybe\n"
        "audioSource=tape\n"
        "unknownKey=1\n");

    CHECK(loaded.presetDurationSeconds == defaults.presetDurationSeconds);
    CHECK(loaded.softCutSeconds == defaults.softCutSeconds);
    CHECK(loaded.shuffle == defaults.shuffle);
    CHECK(loaded.audioSource == defaults.audioSource);
    CHECK(Settings::Parse("").presetDurationSeconds == defaults.presetDurationSeconds);
}

void TestSettingsValuesAreClamped()
{
    CHECK(Settings::Parse("presetDurationSeconds=1\n").presetDurationSeconds == Settings::MinPresetDuration);
    CHECK(Settings::Parse("presetDurationSeconds=99999\n").presetDurationSeconds == Settings::MaxPresetDuration);
    CHECK(Settings::Parse("presetDurationSeconds=90\r\n").presetDurationSeconds == 90);

    Settings settings;
    settings.presetDurationSeconds = Settings::MaxPresetDuration;
    settings.AdjustPresetDuration(Settings::PresetDurationStep);
    CHECK(settings.presetDurationSeconds == Settings::MaxPresetDuration);
    settings.presetDurationSeconds = Settings::MinPresetDuration;
    settings.AdjustPresetDuration(-Settings::PresetDurationStep);
    CHECK(settings.presetDurationSeconds == Settings::MinPresetDuration);
}

void TestFavorites()
{
    Settings settings;
    CHECK(settings.ToggleFavorite("a.milk"));
    CHECK(settings.IsFavorite("a.milk"));
    CHECK(!settings.ToggleFavorite("a.milk"));
    CHECK(!settings.IsFavorite("a.milk"));
    CHECK(Settings::Parse("favorite=a\nfavorite=a\n").favorites.size() == 1);
}

void TestFileNameFromPath()
{
    CHECK(FileNameFromPath("C:\\a\\b c.milk") == "b c.milk");
    CHECK(FileNameFromPath("a/b.milk") == "b.milk");
    CHECK(FileNameFromPath("b.milk") == "b.milk");
    CHECK(FileNameFromPath("").empty());
}

void TestFrameStats()
{
    FrameStats stats(2.0);
    FrameReport report;

    CHECK(!stats.Poll(10.0, report));
    for (int i = 0; i < 120; ++i)
    {
        stats.AddFrame();
    }
    const std::int16_t audio[] = {10, -300, 20, 40};
    stats.AddAudio(audio, 4);

    CHECK(!stats.Poll(11.0, report));
    CHECK(stats.Poll(12.0, report));
    CHECK(report.framesPerSecond > 59.9 && report.framesPerSecond < 60.1);
    CHECK(report.audioFrames == 2);
    CHECK(report.audioPeak == 300);

    const std::int16_t lowest[] = {-32768, 0};
    stats.AddAudio(lowest, 2);
    CHECK(stats.Poll(14.0, report));
    CHECK(report.audioPeak == 32768);
    CHECK(report.framesPerSecond == 0.0);
}

} // namespace

int main()
{
    TestStereoRoundTrip();
    TestMonoIsCopiedToBothChannels();
    TestSurroundKeepsFrontChannels();
    TestOverflowDropsNewFramesAndCountsThem();
    TestWrapsAroundTheEnd();
    TestRejectsInvalidInput();
    TestProducerAndConsumerThreads();
    TestSettingsRoundTrip();
    TestDamagedSettingsKeepDefaults();
    TestSettingsValuesAreClamped();
    TestFavorites();
    TestFileNameFromPath();
    TestFrameStats();

    std::printf(g_failures == 0 ? "All tests passed\n" : "%d checks failed\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
