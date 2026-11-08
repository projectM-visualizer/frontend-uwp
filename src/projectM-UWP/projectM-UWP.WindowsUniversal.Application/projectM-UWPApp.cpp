//*********************************************************
// Designed for use in projectM-UWP Windows Store UWP Application.
// RobertPancoast77@gmail.com
//
//*********************************************************
//
// The app shell: window, lifecycle, input and settings.
//
//*********************************************************

#include "projectM-UWPApp.h"
#include "projectM-UWPPopUp.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <ppltasks.h>
#include <sstream>
#include <vector>

using namespace concurrency;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Media::Devices;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::UI::Core;
using namespace Windows::UI::ViewManagement;

using namespace projectM_UWP;

namespace
{
    const double StatsIntervalSeconds = 10.0;
    const std::size_t AudioBufferFrames = 48000; // One second at the usual mix rate.
    const VirtualKey OemPlus = static_cast<VirtualKey>(187);
    const VirtualKey OemMinus = static_cast<VirtualKey>(189);

    std::string ToUtf8(const std::wstring& text)
    {
        const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size > 0 ? size - 1 : 0, '\0');
        if (size > 1)
        {
            WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], size, nullptr, nullptr);
        }
        return result;
    }

    String^ FromUtf8(const std::string& text)
    {
        const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring result(size > 0 ? size - 1 : 0, L'\0');
        if (size > 1)
        {
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &result[0], size);
        }
        return ref new String(result.c_str());
    }

    double NowSeconds()
    {
        using Clock = std::chrono::steady_clock;
        return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
    }

    // Resolves GL entry points through ANGLE for projectM.
    void* LoadGlProc(const char* name, void* /*userData*/)
    {
        return reinterpret_cast<void*>(eglGetProcAddress(name));
    }
}

// Implementation of the IFrameworkViewSource interface, necessary to run our app.
ref class SimpleApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
    {
        return ref new App();
    }
};

// The main function creates an IFrameworkViewSource for our app, and runs the app.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
    auto simpleApplicationSource = ref new SimpleApplicationSource();
    CoreApplication::Run(simpleApplicationSource);
    return 0;
}

App::App() :
    mStats(StatsIntervalSeconds),
    mAudioBuffer(std::make_shared<Core::AudioRingBuffer>(AudioBufferFrames)),
    mExperimentalPresets(false),
    mWindowClosed(false),
    mWindowVisible(true),
    mReloadPresets(false),
    mCaptureActive(false),
    mCaptureStartPending(false),
    mCaptureRestartRequested(false),
    mCaptureError(S_OK),
    mCaptureRetries(0),
    mCaptureRetryAt(0),
    mMediaFoundationStarted(false),
    mDiscontinuityCount(0)
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
    applicationView->Activated +=
        ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

    CoreApplication::Suspending +=
        ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);
    CoreApplication::Resuming +=
        ref new EventHandler<Object^>(this, &App::OnResuming);

    MediaDevice::DefaultAudioRenderDeviceChanged +=
        ref new TypedEventHandler<Object^, DefaultAudioRenderDeviceChangedEventArgs^>(this, &App::OnDefaultRenderDeviceChanged);
    MediaDevice::DefaultAudioCaptureDeviceChanged +=
        ref new TypedEventHandler<Object^, DefaultAudioCaptureDeviceChangedEventArgs^>(this, &App::OnDefaultCaptureDeviceChanged);
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
    window->VisibilityChanged +=
        ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);
    window->SizeChanged +=
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnSizeChanged);
    window->Closed +=
        ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);
    // https://docs.microsoft.com/en-us/uwp/api/windows.ui.core.corewindow.keydown
    window->KeyDown +=
        ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnWindowKeyDown);

    // The CoreWindow has been created, so EGL can be initialized.
    mSurface.Initialize(window);
}

// Initializes scene resources
void App::Load(String^ entryPoint)
{
    mLocalFolder = ApplicationData::Current->LocalFolder->Path->Data();
    mLogPath = mLocalFolder + L"\\projectM-UWP.log";
    std::ofstream(mLogPath, std::ios::trunc).close();

    LoadSettings();
    CreateRenderer();
    mCaptureStartPending = true;
}

// This method is called after the window becomes active.
void App::Run()
{
    while (!mWindowClosed)
    {
        CoreDispatcher^ dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;

        if (mWindowVisible)
        {
            dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
            ServiceCapture();

            if (mReloadPresets.exchange(false))
            {
                ReloadPresets();
            }

            RenderFrame();
            ReportStats();
        }
        else
        {
            dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }

    // Perform teardown. projectM needs its GL context, so it goes before the surface.
    SaveSettings();
    StopCapture();
    mVisualizer.Destroy();
    mSurface.Cleanup();
    ShutdownCapture();
}

// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

void App::CreateRenderer()
{
    if (!mVisualizer.Create(LoadGlProc, nullptr))
    {
        Log("error: projectM could not start");
        Popup("Unable to start the renderer. This device needs OpenGL ES 3.0 support through Direct3D 11.");
        return;
    }

    // A new projectM instance starts at its default size.
    mViewportWidth = 0;
    mViewportHeight = 0;
    ResetSize();
    ReloadPresets();
}

void App::ReloadPresets()
{
    const std::string installed = ToUtf8(Package::Current->InstalledLocation->Path->Data());
    const std::string imported = ToUtf8(mLocalFolder) + "\\presets\\";
    const std::string bundled = installed + (mExperimentalPresets ? "\\e\\" : "\\p\\");

    mVisualizer.ApplySettings(mSettings);
    mVisualizer.LoadPresets({ bundled, imported }, mSettings);

    Log("presets loaded: " + std::to_string(mVisualizer.PresetCount()));
}

void App::RenderFrame()
{
    ResetSize();
    mVisualizer.RenderFrame(*mAudioBuffer, mStats);
    mStats.AddFrame();

    // If the graphics device was lost, EGL and the GL resources must be rebuilt.
    if (!mSurface.Present())
    {
        RecoverFromDeviceLost();
    }
}

void App::RecoverFromDeviceLost()
{
    Log("graphics device lost, recreating the renderer");

    mVisualizer.Destroy();
    mSurface.Cleanup();
    mSurface.Initialize(CoreWindow::GetForCurrentThread());
    CreateRenderer();
}

// ANGLE resizes its swap chain on the next present, not when the window's
// SizeChanged event fires, so the surface size is also checked every frame.
void App::ResetSize()
{
    const int width = mSurface.Width();
    const int height = mSurface.Height();
    if (width != mViewportWidth || height != mViewportHeight)
    {
        mViewportWidth = width;
        mViewportHeight = height;
        mVisualizer.SetViewport(width, height);
    }
}

// Application lifecycle event handler.
void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
    // Run() won't start until the CoreWindow is activated.
    CoreWindow::GetForCurrentThread()->Activate();
}

// Audio stops while suspended, so that nothing queues up. ANGLE trims its own
// graphics memory on suspend, which is a Store certification requirement.
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ args)
{
    Log("suspending");
    SaveSettings();
    StopCapture();
}

void App::OnResuming(Object^ sender, Object^ args)
{
    Log("resuming");
    mCaptureStartPending = true;
}

// Window event handlers.
void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
    mWindowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
    mWindowClosed = true;
}

void App::OnSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
    ResetSize();
}

void App::OnWindowKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
    // Ignore key repeat, and input before the renderer is ready.
    if (!mVisualizer.IsCreated() || args->KeyStatus.WasKeyDown)
    {
        return;
    }

    const VirtualKey key = args->VirtualKey;

    if (key == OemPlus)
    {
        ChangePresetDuration(Core::Settings::PresetDurationStep);
        return;
    }
    if (key == OemMinus)
    {
        ChangePresetDuration(-Core::Settings::PresetDurationStep);
        return;
    }

    // https://support.xbox.com/en-US/xbox-one/accessories/xbox-one-wireless-controller
    switch (key)
    {
    case VirtualKey::R:
    case VirtualKey::Enter:
    case VirtualKey::GamepadA:
    case VirtualKey::GamepadX:
    case VirtualKey::GamepadY:
        mVisualizer.RandomPreset();
        break;

    case VirtualKey::GamepadLeftShoulder:
    case VirtualKey::GamepadRightShoulder:
    case VirtualKey::Space:
    case VirtualKey::Control:
        mVisualizer.TogglePresetLock();
        break;

    case VirtualKey::GamepadDPadRight:
    case VirtualKey::Right:
    case VirtualKey::GamepadRightTrigger:
    case VirtualKey::N:
        mVisualizer.NextPreset();
        break;

    case VirtualKey::GamepadDPadLeft:
    case VirtualKey::Left:
    case VirtualKey::GamepadLeftTrigger:
    case VirtualKey::P:
        mVisualizer.PreviousPreset();
        break;

    case VirtualKey::GamepadDPadDown:
    case VirtualKey::Down:
        ToggleExperimentalPresets();
        break;

    case VirtualKey::F:
        if (IsCtrlKeyPressed() || IsAltKeyPressed())
        {
            ToggleFullscreen();
        }
        break;

    case VirtualKey::L:
        ToggleFavorite();
        break;

    case VirtualKey::V:
        ToggleFavoritesOnly();
        break;

    case VirtualKey::I:
        ImportPresets();
        break;

    case VirtualKey::M:
        ToggleAudioSource();
        break;

    case VirtualKey::Add:
        ChangePresetDuration(Core::Settings::PresetDurationStep);
        break;

    case VirtualKey::Subtract:
        ChangePresetDuration(-Core::Settings::PresetDurationStep);
        break;

    case VirtualKey::GamepadView:
    case VirtualKey::GamepadMenu:
    case VirtualKey::F1:
    case VirtualKey::Escape:
        ShowHelp();
        break;

    default:
        break;
    }
}

bool App::IsCtrlKeyPressed()
{
    auto state = CoreWindow::GetForCurrentThread()->GetKeyState(VirtualKey::Control);
    return (state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
}

// The Alt key is represented by the VirtualKey.Menu value.
bool App::IsAltKeyPressed()
{
    auto state = CoreWindow::GetForCurrentThread()->GetKeyState(VirtualKey::Menu);
    return (state & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
}

void App::ToggleFullscreen()
{
    auto applicationView = ApplicationView::GetForCurrentView();

    if (applicationView->IsFullScreenMode)
    {
        applicationView->ExitFullScreenMode();
    }
    else
    {
        applicationView->TryEnterFullScreenMode();
    }
}

void App::ToggleExperimentalPresets()
{
    mExperimentalPresets = !mExperimentalPresets;
    ReloadPresets();
    Popup("Experimental Presets : " + mExperimentalPresets.ToString());
}

void App::ToggleFavorite()
{
    const std::string name = Core::FileNameFromPath(mVisualizer.CurrentPresetPath());
    if (name.empty())
    {
        return;
    }

    const bool favorite = mSettings.ToggleFavorite(name);
    SaveSettings();
    Popup((favorite ? "Added to favorites: " : "Removed from favorites: ") + FromUtf8(name));
}

void App::ToggleFavoritesOnly()
{
    if (!mSettings.favoritesOnly && mSettings.favorites.empty())
    {
        Popup("No favorites yet. Press L to add the current preset.");
        return;
    }

    mSettings.favoritesOnly = !mSettings.favoritesOnly;
    SaveSettings();
    ReloadPresets();
    Popup(mSettings.favoritesOnly ? "Playing favorites only" : "Playing all presets");
}

void App::ToggleAudioSource()
{
    const bool toMicrophone = mSettings.audioSource == Core::AudioSourceKind::SystemAudio;
    mSettings.audioSource = toMicrophone ? Core::AudioSourceKind::Microphone : Core::AudioSourceKind::SystemAudio;
    SaveSettings();

    mCaptureRestartRequested = true;
    Popup(toMicrophone ? "Audio source: microphone" : "Audio source: system audio");
}

void App::ChangePresetDuration(int deltaSeconds)
{
    mSettings.AdjustPresetDuration(deltaSeconds);
    mVisualizer.ApplySettings(mSettings);
    SaveSettings();
    Popup("Preset duration: " + mSettings.presetDurationSeconds.ToString() + " seconds");
}

// Copies the chosen files into the app's own folder, which is the only place a
// sandboxed app can keep reading them from after this picker closes.
void App::ImportPresets()
{
    auto picker = ref new FileOpenPicker();
    picker->ViewMode = PickerViewMode::List;
    picker->SuggestedStartLocation = PickerLocationId::DocumentsLibrary;
    picker->FileTypeFilter->Append(".milk");
    picker->FileTypeFilter->Append(".prjm");

    create_task(picker->PickMultipleFilesAsync()).then([this](IVectorView<StorageFile^>^ files)
    {
        if (files->Size == 0)
        {
            cancel_current_task();
        }

        return create_task(ApplicationData::Current->LocalFolder->CreateFolderAsync("presets", CreationCollisionOption::OpenIfExists))
            .then([files](StorageFolder^ folder)
        {
            std::vector<task<StorageFile^>> copies;
            for (StorageFile^ file : files)
            {
                copies.push_back(create_task(file->CopyAsync(folder, file->Name, NameCollisionOption::ReplaceExisting)));
            }
            return when_all(copies.begin(), copies.end());
        });
    }).then([this](task<std::vector<StorageFile^>> result)
    {
        try
        {
            Log("imported presets: " + std::to_string(result.get().size()));
            mReloadPresets = true;
        }
        catch (const task_canceled&)
        {
            // The picker was closed without choosing anything.
        }
        catch (Exception^ e)
        {
            Log("preset import failed: " + ToUtf8(e->Message->Data()));
        }
    });
}

void App::ShowHelp()
{
    std::string text = mVisualizer.IsPresetLocked() ? "PresetEyeD == LOCKED: " : "PresetEyeD == ";
    text += Core::FileNameFromPath(mVisualizer.CurrentPresetPath());
    text +=
        "\n"
        "This help menu : F1 or ESC : GamepadMenu or GamepadView\n"
        "Lock/Unlock Preset : SPACE or CTRL : GamepadRightShoulder or GamepadLeftShoulder\n"
        "Next preset : RIGHT or N : GamepadRightTrigger or GamepadDPadRight\n"
        "Previous preset : LEFT or P : GamepadLeftTrigger or GamepadDPadLeft\n"
        "Random preset : ENTER or R : GamepadA or GamepadX or GamepadY\n"
        "Toggle Fullscreen (PC Only) : CTRL-F\n"
        "Experimental Presets : DOWN : GamepadDPadDown\n"
        "Add or remove favorite : L\n"
        "Favorites only : V\n"
        "Import presets : I\n"
        "Preset duration : + or -\n"
        "System audio or microphone : M\n";

    Popup(FromUtf8(text));
}

void App::ReportAudioUnavailable()
{
    Popup("No audio device is available, so the visuals won't react to sound. projectM-UWP will try again when the default audio device changes.");
}

void App::LoadSettings()
{
    std::ifstream file(mLocalFolder + L"\\settings.txt");
    std::ostringstream text;
    text << file.rdbuf();
    mSettings = Core::Settings::Parse(text.str());
}

void App::SaveSettings()
{
    std::ofstream file(mLocalFolder + L"\\settings.txt", std::ios::trunc);
    file << mSettings.Serialize();
}

void App::Log(const std::string& line)
{
    std::ofstream file(mLogPath, std::ios::app);
    file << line << '\n';
}

// Writes measurements to the local folder, since release builds have no console.
void App::ReportStats()
{
    Core::FrameReport report;
    if (!mStats.Poll(NowSeconds(), report))
    {
        return;
    }

    char line[160];
    std::snprintf(line, sizeof(line), "fps=%.1f audioFrames=%llu peak=%d dropped=%llu memoryMB=%llu presets=%zu",
        report.framesPerSecond,
        static_cast<unsigned long long>(report.audioFrames),
        report.audioPeak,
        static_cast<unsigned long long>(mAudioBuffer->DroppedFrames()),
        static_cast<unsigned long long>(MemoryManager::AppMemoryUsage / (1024 * 1024)),
        mVisualizer.PresetCount());
    Log(line);
}
