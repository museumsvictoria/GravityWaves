//
//  WMFVideoPlayerPrivateImpl.h
//  SFMoma
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#ifndef SFMoma_WMFVideoPlayerPrivateImpl_h
#define SFMoma_WMFVideoPlayerPrivateImpl_h

#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <evr.h>
#include <memory>
#include <functional>

#include <Video/Win32/WMF/WMFVideoPlayerTopology.h>
#include <Video/Win32/WMF/Presenter/EVRPresenter.h>
#include "cinder/Cinder.h"


namespace Video
{		
    template <class T> void SafeRelease ( T ** t )
    {
        if ( *t )
        {
            (*t)->Release();
            *t = nullptr;
        }
    }

    const UINT WM_APP_PLAYER_EVENT = WM_APP + 1;   
  
    enum class PlayerState
    {
        Closed = 0,			// No session.
        Ready,				// Session was created, ready to open a file. 
	    OpenAsyncPending,	// Session is creating URL resource
	    OpenAsyncComplete,	// Session finished opening URL
        OpenPending,		// Session is opening a file.
        Started,			// Session is playing a file.
        Paused,				// Session is paused.
        Stopped,			// Session is stopped (ready to play). 
        Closing,			// Application has closed the session, but is waiting for MESessionClosed.
		Seeking
    };

    class WMFVideoPlayerPrivateImpl
    : public IMFAsyncCallback
	{
        friend class WMFVideoPlayerImpl;
    public:

		using CompleteFn =						std::function < void() > ;

        static HRESULT                          CreateInstance      ( HWND videoWindow, HWND eventWindow, WMFVideoPlayerPrivateImpl ** player );

        STDMETHODIMP                            QueryInterface      ( REFIID id, void ** v ) override;
        STDMETHODIMP_(ULONG)                    AddRef              ( ) override;
        STDMETHODIMP_(ULONG)                    Release             ( ) override;
        STDMETHODIMP                            GetParameters       ( DWORD *, DWORD * ) override;
        STDMETHODIMP                            Invoke              ( IMFAsyncResult * result ) override;

        HRESULT                                 OpenURL             ( const WCHAR * sURL, const WCHAR * audioDeviceId = 0 );
	    HRESULT			                        EndOpenURL          ( const WCHAR * audioDeviceId = 0 );

        HRESULT                                 Play                ( );
        HRESULT                                 Pause               ( );
        HRESULT                                 Stop                ( );
        HRESULT                                 Shutdown            ( );
        HRESULT                                 GetBufferProgress   ( DWORD * progress );
        
        HRESULT		                            PlaybackRate        ( float rateRequested, BOOL bThin = false );
	    float		                            PlaybackRate        ( );
        
        void                                    Volume              ( float volume );
        float                                   Volume              ( ) const;

        float                                   Duration            ( );
        float                                   Time                ( );
        bool                                    SeekToTime          ( float time );

        float                                   FPS                 ( ) const;

        int                                     Width               ( ) const;
        int                                     Height              ( ) const;

        EVRCustomPresenter *                    Presenter           ( ) const;
        
        PlayerState                             GetState            ( ) const;
        bool                                    HasVideo            ( ) const;

		void									OnComplete			( CompleteFn handler );
		bool									IsLooping{false};
                
    protected:

        WMFVideoPlayerPrivateImpl               ( HWND videoWindow, HWND eventWindow );
        virtual ~WMFVideoPlayerPrivateImpl      ( );

        HRESULT                                 Initialize          ( );
        HRESULT                                 CreateSession       ( );
        HRESULT                                 CloseSession        ( );
        HRESULT                                 StartPlayback       ( );
        HRESULT                                 HandleEvent         ( UINT_PTR eventPtr );
        HRESULT                                 SetMediaInfo        ( IMFPresentationDescriptor * info );

        virtual HRESULT                         OnTopologyStatus    ( IMFMediaEvent * event );
        virtual HRESULT                         OnPresentationEnded ( IMFMediaEvent * event );
        virtual HRESULT                         OnNewPresentation   ( IMFMediaEvent * event );
        virtual HRESULT                         OnSessionEvent      ( IMFMediaEvent * event, MediaEventType type );

        long                                    _refCount{1};
        int                                     _numFrames{0};
        ci::vec2                                _size{0};
        float                                   _volume{1.0f};
        float                                   _fps{10000.0f};

        IMFSequencerSource *                    _sequencerSource{nullptr};
        IMFSourceResolver *                     _sourceResolver{nullptr};
        IMFMediaSource *                        _source{nullptr};
        IMFVideoDisplayControl *                _videoDisplay{nullptr};
        IMFAudioStreamVolume *                  _volumeControl{nullptr};
        MFSequencerElementId                    _previousTopographyID{0};
        IMFMediaSession *                       _session{nullptr};
        
        EVRCustomPresenter *                    _presenter{nullptr};
       
        HWND                                    _videoWindow{nullptr};
        HWND                                    _eventWindow{nullptr};

        bool                                    _isFinished{false};
        
		CompleteFn								_onComplete;
        HANDLE                                  _closeEvent{nullptr};      
        PlayerState                             _state{PlayerState::Closed};
		PlayerState								_afterSeekState{ PlayerState::Closed };
		float									_lastKnownTime{ 0.0f };
	};
}

#endif