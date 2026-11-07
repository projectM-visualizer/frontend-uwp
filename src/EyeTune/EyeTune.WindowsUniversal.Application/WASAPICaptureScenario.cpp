//*********************************************************
// Designed for use in EyeTune Windows Store UWP Application.
// RobertPancoast77@gmail.com
//
//*********************************************************
//
// Starts, stops and recovers audio capture for the app.
//
// Capture is independent of the renderer. It writes to a buffer that outlives both,
// so recreating the renderer doesn't touch it. Capture threads only set flags, and
// ServiceCapture acts on them from the main loop, because activating an audio
// device has to happen on the UI thread.
//
//*********************************************************

#include "EyeTuneApp.h"

#include <mfapi.h>

using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::Media::Devices;

using namespace EyeTune;

namespace
{
    // A device can be briefly missing, for example while a remote session connects.
    const int MaxCaptureRetries = 5;
    const unsigned long long CaptureRetryDelayMs = 3000;
}

// Event callback from WASAPI capture for changes in device state.
void App::OnDeviceStateChange(Object^ sender, DeviceStateChangedEventArgs^ e)
{
    switch (e->State)
    {
    case DeviceState::DeviceStateInitialized:
        mCapture->StartCaptureAsync();
        break;

    case DeviceState::DeviceStateCapturing:
        mCaptureRetries = 0;
        Log("audio capture started");
        break;

    case DeviceState::DeviceStateDiscontinuity:
        // There is always one when capture starts, so only later ones are unusual.
        if (++mDiscontinuityCount > 1)
        {
            Log("audio discontinuity");
        }
        break;

    case DeviceState::DeviceStateStopped:
    case DeviceState::DeviceStateInError:
        if (e->State == DeviceState::DeviceStateInError)
        {
            mCaptureError = e->hr;
        }

        if (mDeviceStateChangeToken.Value != 0)
        {
            mStateChangedEvent->StateChangedEvent -= mDeviceStateChangeToken;
            mStateChangedEvent = nullptr;
            mDeviceStateChangeToken.Value = 0;
        }

        // For the stopped state, completely tear down the audio device.
        mCapture = nullptr;
        mCaptureActive = false;
        break;

    default:
        break;
    }
}

void App::OnDefaultRenderDeviceChanged(Object^ sender, DefaultAudioRenderDeviceChangedEventArgs^ args)
{
    if (args->Role == AudioDeviceRole::Default && mSettings.audioSource == Core::AudioSourceKind::SystemAudio)
    {
        mCaptureRestartRequested = true;
    }
}

void App::OnDefaultCaptureDeviceChanged(Object^ sender, DefaultAudioCaptureDeviceChangedEventArgs^ args)
{
    if (args->Role == AudioDeviceRole::Default && mSettings.audioSource == Core::AudioSourceKind::Microphone)
    {
        mCaptureRestartRequested = true;
    }
}

// Runs once per frame on the UI thread.
void App::ServiceCapture()
{
    if (mCaptureRestartRequested.exchange(false))
    {
        Log("restarting audio capture");
        mCaptureRetries = 0;
        StopCapture();
        mCaptureStartPending = true;
    }

    const HRESULT error = mCaptureError.exchange(S_OK);
    if (FAILED(error))
    {
        wchar_t code[11];
        swprintf_s(code, L"0x%08x", static_cast<unsigned int>(error));
        Log("audio capture failed: " + std::string(code, code + 10));

        if (++mCaptureRetries <= MaxCaptureRetries)
        {
            mCaptureRetryAt = GetTickCount64() + CaptureRetryDelayMs;
        }
        else
        {
            ReportAudioUnavailable();
        }
    }

    if (mCaptureRetryAt != 0 && GetTickCount64() >= mCaptureRetryAt)
    {
        mCaptureRetryAt = 0;
        mCaptureStartPending = true;
    }

    if (!mCaptureActive && mCaptureStartPending.exchange(false))
    {
        InitializeCapture();
    }
}

void App::InitializeCapture()
{
    if (!mMediaFoundationStarted)
    {
        ThrowIfFailed(MFStartup(MF_VERSION, MFSTARTUP_LITE));
        mMediaFoundationStarted = true;
    }

    mCapture = Make<WASAPICapture>();
    if (nullptr == mCapture)
    {
        return;
    }

    mStateChangedEvent = mCapture->GetDeviceStateEvent();
    if (nullptr == mStateChangedEvent)
    {
        mCapture = nullptr;
        return;
    }

    mDeviceStateChangeToken = mStateChangedEvent->StateChangedEvent +=
        ref new DeviceStateChangedHandler(this, &App::OnDeviceStateChange);

    mDiscontinuityCount = 0;
    mCaptureActive = true;

    CAPTUREDEVICEPROPS props;
    props.IsLowLatency = false;
    props.UseMicrophone = mSettings.audioSource == Core::AudioSourceKind::Microphone;
    mCapture->SetProperties(props);

    mCapture->InitializeAudioDeviceAsync(mAudioBuffer);
}

void App::StopCapture()
{
    if (mCapture)
    {
        mCapture->StopCaptureAsync();
    }
}

void App::ShutdownCapture()
{
    if (mMediaFoundationStarted)
    {
        MFShutdown();
        mMediaFoundationStarted = false;
    }
}
