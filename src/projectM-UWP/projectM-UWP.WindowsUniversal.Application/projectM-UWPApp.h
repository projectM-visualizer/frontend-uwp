//*********************************************************
// Designed for use in projectM-UWP Windows Store UWP Application.
// RobertPancoast77@gmail.com
//
//*********************************************************
//
// The app shell: window, lifecycle, input and settings. Rendering lives in
// AngleSurface and Visualizer, and audio capture in WASAPICapture.
//
//*********************************************************

#pragma once

#include "AngleSurface.h"
#include "AudioRingBuffer.h"
#include "FrameStats.h"
#include "Settings.h"
#include "Visualizer.h"
#include "WASAPICapture.h"

#include <atomic>
#include <memory>
#include <string>
#include <wrl.h>

namespace projectM_UWP
{
    ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
    {
    public:
        App();

        // IFrameworkView Methods.
        virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
        virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
        virtual void Load(Platform::String^ entryPoint);
        virtual void Run();
        virtual void Uninitialize();

    private:
        // Renderer
        void CreateRenderer();
        void ReloadPresets();
        void ResetSize();
        void RecoverFromDeviceLost();
        void RenderFrame();

        // Application lifecycle event handlers.
        void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
        void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
        void OnResuming(Platform::Object^ sender, Platform::Object^ args);

        // Window event handlers.
        void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
        void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);
        void OnWindowKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
        void OnSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);

        // Commands
        void ToggleFullscreen();
        void ToggleExperimentalPresets();
        void ToggleFavorite();
        void ToggleFavoritesOnly();
        void ToggleAudioSource();
        void ChangePresetDuration(int deltaSeconds);
        void ImportPresets();
        void ShowHelp();
        bool IsCtrlKeyPressed();
        bool IsAltKeyPressed();

        // Audio, defined in WASAPICaptureScenario.cpp
        void OnDeviceStateChange(Platform::Object^ sender, DeviceStateChangedEventArgs^ e);
        void OnDefaultRenderDeviceChanged(Platform::Object^ sender, Windows::Media::Devices::DefaultAudioRenderDeviceChangedEventArgs^ args);
        void OnDefaultCaptureDeviceChanged(Platform::Object^ sender, Windows::Media::Devices::DefaultAudioCaptureDeviceChangedEventArgs^ args);
        void InitializeCapture();
        void StopCapture();
        void ShutdownCapture();
        void ServiceCapture();
        void ReportAudioUnavailable();

        // Settings and diagnostics
        void LoadSettings();
        void SaveSettings();
        void Log(const std::string& line);
        void ReportStats();

        AngleSurface mSurface;
        Visualizer mVisualizer;
        Core::Settings mSettings;
        Core::FrameStats mStats;
        std::shared_ptr<Core::AudioRingBuffer> mAudioBuffer;

        std::wstring mLocalFolder;
        std::wstring mLogPath;
        bool mExperimentalPresets;
        bool mWindowClosed;
        bool mWindowVisible;
        std::atomic<bool> mReloadPresets;

        // Capture runs on its own threads. It reports back through these flags and
        // the main loop acts on them, because restarts must happen on the UI thread.
        std::atomic<bool> mCaptureActive;
        std::atomic<bool> mCaptureStartPending;
        std::atomic<bool> mCaptureRestartRequested;
        std::atomic<long> mCaptureError;
        std::atomic<int> mCaptureRetries;
        unsigned long long mCaptureRetryAt;

        bool mMediaFoundationStarted;
        int mDiscontinuityCount;
        Windows::Foundation::EventRegistrationToken mDeviceStateChangeToken;
        DeviceStateChangedEvent^ mStateChangedEvent;
        Microsoft::WRL::ComPtr<WASAPICapture> mCapture;
    };
}
