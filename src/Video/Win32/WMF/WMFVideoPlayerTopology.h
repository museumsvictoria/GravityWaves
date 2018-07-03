//
//  WMFVideoPlayerTopology.h
//  SFMoma
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#ifndef SFMoma_WMFVideoPlayerTopology_h
#define SFMoma_WMFVideoPlayerTopology_h

#include <Windows.h>
#include <mmdeviceapi.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr.h>

namespace Video
{
    HRESULT CreatePlaybackTopology ( IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, HWND hVideoWnd, IMFTopology **ppTopology, IMFVideoPresenter *pVideoPresenter, const WCHAR *audioDeviceId = 0 );
    HRESULT AddToPlaybackTopology  ( IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, HWND hVideoWnd, IMFTopology *pTopology, IMFVideoPresenter *pVideoPresenter );
}

#endif /* SFMoma_WMFVideoPlayerTopology_h */