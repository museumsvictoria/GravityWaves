//
//  WMFMediaPlayerPrivateImpl.cxx
//  SFMoma
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#include <Windows.h>
#include <Shlwapi.h>
#include <Video/Win32/WMF/WMFVideoPlayerImpl.h>
#include <Video/Win32/WMF/WMFVideoPlayerPrivateImpl.h>
#include <Video/Win32/WMF/Presenter/EVRPresenter.h>


#pragma comment(lib, "shlwapi")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Propsys.lib")

using namespace ci;

#ifndef NDEBUG
#define lucy_assert(x, y) assert(x)
#else
#define lucy_assert(x, y)
#endif

namespace
{
    template <class Q>
    HRESULT GetEventObject ( IMFMediaEvent *pEvent, Q **ppObject )
    {
        *ppObject = nullptr;

        PROPVARIANT var;
        HRESULT hr = pEvent->GetValue(&var);
        if (SUCCEEDED(hr))
        {
            if (var.vt == VT_UNKNOWN)
            {
                hr = var.punkVal->QueryInterface(ppObject);
            }
            else
            {
                hr = MF_E_INVALIDTYPE;
            }
            PropVariantClear(&var);
        }
        return hr;
    }
}

namespace Video
{
    HRESULT WMFVideoPlayerPrivateImpl::CreateInstance ( HWND videoWindow, HWND eventWindow, WMFVideoPlayerPrivateImpl ** player )
    {
        HRESULT result = S_OK;
        auto instance = new WMFVideoPlayerPrivateImpl ( videoWindow, eventWindow );
        if ( SUCCEEDED ( result = instance->Initialize() ) )
        {
           *player = instance;
        }else
        {
            instance->Release();
        }

        return result;
    }

    STDMETHODIMP WMFVideoPlayerPrivateImpl::QueryInterface ( REFIID id, void ** v )
    {
        static const QITAB qit[] = { QITABENT(WMFVideoPlayerPrivateImpl, IMFAsyncCallback), { 0 } };
        return QISearch(this, qit, id, v);
    }

    STDMETHODIMP_(ULONG) WMFVideoPlayerPrivateImpl::AddRef ( )
    {
        return InterlockedIncrement(&_refCount);
    }

    STDMETHODIMP_(ULONG) WMFVideoPlayerPrivateImpl::Release ( )
    {
        auto count = InterlockedDecrement(&_refCount);
        if ( count == 0 )
        {
            delete this;
        }
        return count;
    }

    STDMETHODIMP WMFVideoPlayerPrivateImpl::GetParameters ( DWORD *, DWORD * )
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP WMFVideoPlayerPrivateImpl::Invoke ( IMFAsyncResult * asyncResult )
    {
        MediaEventType eventType = MEUnknown;
        IMFMediaEvent * mediaEvent = nullptr;

        HRESULT result;

        if ( !_session )
        {
            Error ( "Invalid session!" );
            return -1;
        }

        auto cleanup = [&] ( ) -> HRESULT
        {
            SafeRelease ( &mediaEvent );
            return S_OK;
        };

        if ( _state == PlayerState::OpenAsyncPending )
        {
            // @TODO (andrew): Source has !&_sourceResolver
            if ( !_sourceResolver )
            {
                Error ( "Async request returned with null session" );
                return -1;
            }

            MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
            IUnknown * sourceUnknown  = nullptr;

            result = _sourceResolver->EndCreateObjectFromURL ( asyncResult, &objectType, &sourceUnknown );
            if ( SUCCEEDED ( result ) )
            {
                result = sourceUnknown->QueryInterface ( __uuidof ( IMFMediaSource ), ( void ** ) ( &_source ) );
            }
            SafeRelease ( &sourceUnknown );
            return result;
        }

        result = _session->EndGetEvent ( asyncResult, &mediaEvent );
        if ( FAILED(result) )
        {
            return cleanup();
        }

        result = mediaEvent->GetType(&eventType);
        if ( FAILED(result) )
        {
            return cleanup();
        }

        if (eventType == MESourceSeeked || eventType == MEStreamSeeked)
        {
            Info("GOT SEEK!");
        }

        if ( eventType == MESessionClosed )
        {
            SetEvent ( _closeEvent );
        }else
        {
            result = _session->BeginGetEvent ( this, nullptr );

            if ( FAILED ( result ) )
            {
                return cleanup();
            }
        }

        if ( _state != PlayerState::Closing )
        {
            mediaEvent->AddRef();
            PostMessageW ( _eventWindow, WM_APP_PLAYER_EVENT, (WPARAM)mediaEvent, (LPARAM)eventType );
        }

        return cleanup();
    }

    HRESULT WMFVideoPlayerPrivateImpl::OpenURL ( const WCHAR * sURL, const WCHAR * audioDeviceId )
    {
        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            if ( FAILED ( r ) )
            {
                _state = PlayerState::Closed;

                switch ( r )
                {
                    case MF_E_SOURCERESOLVER_MUTUALLY_EXCLUSIVE_FLAGS :
                    {
                        Error("MF_E_SOURCERESOLVER_MUTUALLY_EXCLUSIVE_FLAGS"); 
                        break;
                    }

                    case MF_E_UNSUPPORTED_SCHEME :
                    {
                        Error("MF_E_UNSUPPORTED_SCHEME"); 
                        break;
                    }

                    case MF_E_UNSUPPORTED_BYTESTREAM_TYPE :
                    {
                        Error("MF_E_UNSUPPORTED_BYTESTREAM_TYPE"); 
                        break;
                    }

                    default:
                        Error("Unknown eror"); 
                        break;
                }
            }

            return r;
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup(hr);

        HRESULT result = CreateSession();
        CHECK( result );

        result = CreateMediaSource( sURL, &_source);
        CHECK( result );

        EndOpenURL( audioDeviceId );

        return result;
    }
    
    HRESULT WMFVideoPlayerPrivateImpl::EndOpenURL ( const WCHAR * audioDeviceId )
    {
        HRESULT result;

        IMFTopology *pTopology = nullptr;
        IMFPresentationDescriptor* pSourcePD = nullptr;

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            if (FAILED(r))
            {
                _state = PlayerState::Closed;
            }

            SafeRelease(&pSourcePD);
            SafeRelease(&pTopology);
            return r;    
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup(hr);

        result = _source->CreatePresentationDescriptor(&pSourcePD);
        CHECK( result );

        result = CreatePlaybackTopology(_source, pSourcePD, _videoWindow, &pTopology, _presenter, audioDeviceId);
        CHECK( result );

        SetMediaInfo(pSourcePD);
        
        result = _session->SetTopology(0, pTopology);
        CHECK( result );

        _state = PlayerState::OpenPending;
        _volume = 1.0f;

        _isFinished = false;

        return cleanup(result);
    }

    HRESULT WMFVideoPlayerPrivateImpl::Play ( )
    {
        _lastKnownTime = 0.0f;

        if ( _state != PlayerState::Paused && _state != PlayerState::Stopped )
        {
            return MF_E_INVALIDREQUEST;     
        }

        if ( _session == nullptr || _source == nullptr )
        {
            return E_UNEXPECTED;
        }

        return StartPlayback();
    }

    HRESULT WMFVideoPlayerPrivateImpl::Pause ( )
    {
        if ( _state != PlayerState::Started )
        {
            return MF_E_INVALIDREQUEST;
        }

        if ( !_session || !_source )
        {
            return E_UNEXPECTED;
        }

        HRESULT result = _session->Pause ( );
        if ( SUCCEEDED ( result ) ) 
        {
            _state = PlayerState::Paused;
        }

        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::Stop ( )
    {
        if ( _state != PlayerState::Started && _state != PlayerState::Paused )
        {
            return MF_E_INVALIDREQUEST;
        }

        if ( !_session )
        {
            return E_UNEXPECTED;
        }

        HRESULT result = _session->Stop ( );
        if ( SUCCEEDED ( result ) )
        {
           _state = PlayerState::Stopped;
        }

        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::Shutdown ( )
    {
        HRESULT result = S_OK;

        result = CloseSession ( );

        if ( _closeEvent )
        {
            CloseHandle ( _closeEvent );
            _closeEvent = nullptr;
        }

        if ( _presenter )
        {
            _presenter->ReleaseSharedTexture ( );
        }

        SafeRelease ( &_presenter );
        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::GetBufferProgress ( DWORD * progress )
    {
        IPropertyStore * store = nullptr;
        PROPVARIANT prop;
        PropVariantInit(&prop);

        HRESULT result = MFGetService ( _session, MFNETSOURCE_STATISTICS_SERVICE, IID_PPV_ARGS( &store ) );

        if ( SUCCEEDED ( result ) )
        {
            PROPERTYKEY key;
            key.fmtid = MFNETSOURCE_STATISTICS;
            key.pid = MFNETSOURCE_BUFFERPROGRESS_ID;

            result = store->GetValue ( key, &prop );
        }

        if ( SUCCEEDED ( result ) )
        {
            *progress = prop.lVal;
        }

        PropVariantClear ( &prop );
        SafeRelease ( &store );

        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::PlaybackRate ( float rateRequested, BOOL bThin )
    {
        HRESULT result = S_OK;
        IMFRateControl * rateControl = nullptr;

        result = MFGetService ( _session, MF_RATE_CONTROL_SERVICE, IID_IMFRateControl, (void **)&rateControl );
        if ( SUCCEEDED ( result ) )
        {
            result = rateControl->SetRate ( bThin, rateRequested );
        }

        SafeRelease ( &rateControl );

        return S_OK;
    }
        
    float WMFVideoPlayerPrivateImpl::PlaybackRate ( )
    {
        HRESULT result = S_OK;
        IMFRateControl * rateControl = nullptr;
        BOOL thinning = false;
        float rate = 1.0f;

        result = MFGetService ( _session, MF_RATE_CONTROL_SERVICE, IID_IMFRateControl, (void **) &rateControl );

        if ( SUCCEEDED ( result ) )
        {
            result = rateControl->GetRate ( &thinning, &rate );
        }

        SafeRelease ( &rateControl );
        
        if ( SUCCEEDED ( result ) )
        {
            return rate;
        }

        Error ( "Error retrieving playback rate" );
        return 1.0f;
    }

    void WMFVideoPlayerPrivateImpl::Volume ( float volume )
    {
        _volume = volume;

        if ( !_session )
        {
            Error ( "Attempting to set volume on an invalid session!" );
            return;
        }

        if ( !_volumeControl )
        {
            HRESULT result = MFGetService ( _session, MR_STREAM_VOLUME_SERVICE, __uuidof ( IMFAudioStreamVolume ), (void **)&_volumeControl );
            if ( FAILED ( result ) )
            {
                Error ( "Error aquiring sound control interface" );
                return ;
            }
        }

        UINT32 numChannels = 0;
        _volumeControl->GetChannelCount ( &numChannels );
        for ( int i = 0; i < numChannels; i++ )
        {
            _volumeControl->SetChannelVolume ( i, volume );
        }

        //Info("Volume successfully set to " + std::to_string(_volume));
    }
    
    float WMFVideoPlayerPrivateImpl::Volume ( ) const
    {
        return _volume;
    }

    float WMFVideoPlayerPrivateImpl::Duration ( )
    {
        float duration = 0.0f;
        if ( !_source ) return 0.0f;

        IMFPresentationDescriptor * descriptor = nullptr;
        HRESULT result = _source->CreatePresentationDescriptor ( &descriptor );
        if ( SUCCEEDED ( result ) ) 
        {
            UINT64 longDuration = 0;
            result = descriptor->GetUINT64(MF_PD_DURATION, &longDuration);
            if (SUCCEEDED(result))
                duration = (float)longDuration / 10000000.0;
        }
        SafeRelease(&descriptor);
        return duration;
    }
    
    float WMFVideoPlayerPrivateImpl::Time ( )
    {
        if ( _state != PlayerState::Started ) return _lastKnownTime;

        float position = _lastKnownTime;
        if (_session == nullptr) return _lastKnownTime;

        IMFPresentationClock *clock = nullptr;
        HRESULT result = _session->GetClock((IMFClock **)&clock);
    
        if (SUCCEEDED(result)) 
        {
            MFTIME longPosition = 0;
            result = clock->GetTime(&longPosition);
            if (SUCCEEDED(result))
            {
                position = (float)longPosition / 10000000.0;
                _lastKnownTime = position;
            }
        }

        SafeRelease(&clock);
        return position;
    }

    bool WMFVideoPlayerPrivateImpl::SeekToTime ( float time )
    {
        float d = Duration();
        if (time > d - 1.0f) time = d - 1.0f;
        if (time < 0.0f) time = 0.0f;

        if ( _state == PlayerState::OpenPending )
        {
            Error ( "Can't seek video while it's opening");
            return false; 
        }

        if (_state == PlayerState::Seeking)
        {
            return false;
        }

        //PlaybackRate(0.0f);

        PlayerState currentState = _state;
        
        PROPVARIANT start;
        PropVariantInit ( &start );

        start.vt = VT_I8;
        start.hVal.QuadPart = time * 10000000.0;

        HRESULT result = _session->Start ( &GUID_NULL, &start );
        
        if ( SUCCEEDED ( result ) )
        {
            _state = PlayerState::Seeking;
            _afterSeekState = currentState;
            if ( currentState == PlayerState::Stopped ) Stop();
            if ( currentState == PlayerState::Paused ) Pause();
        }

        PropVariantClear ( &start );
        return SUCCEEDED(result);
    }

    float WMFVideoPlayerPrivateImpl::FPS ( ) const
    {
        return _fps;
    }

    int WMFVideoPlayerPrivateImpl::Width ( ) const
    {
        return _size.x;
    }

    int WMFVideoPlayerPrivateImpl::Height ( ) const
    {
        return _size.y;
    }

    EVRCustomPresenter * WMFVideoPlayerPrivateImpl::Presenter ( ) const
    {
        return _presenter;
    }

    PlayerState WMFVideoPlayerPrivateImpl::GetState ( ) const
    {
        return _state;
    }

    bool WMFVideoPlayerPrivateImpl::HasVideo ( ) const
    {
        return _videoDisplay != nullptr;
    }
    
    void WMFVideoPlayerPrivateImpl::OnComplete(CompleteFn handler)
    {
        _onComplete = handler;
    }

    WMFVideoPlayerPrivateImpl::WMFVideoPlayerPrivateImpl ( HWND videoWindow, HWND eventWindow )
    : _videoWindow(videoWindow)
    , _eventWindow(eventWindow)
    {
    }

    WMFVideoPlayerPrivateImpl::~WMFVideoPlayerPrivateImpl ( )
    {
        assert ( _session == nullptr );

        Shutdown();
        SafeRelease(&_sequencerSource);
    }

    HRESULT WMFVideoPlayerPrivateImpl::Initialize ( )
    {
        HRESULT result = 0;
        
        _closeEvent = CreateEventA ( nullptr, false, false, nullptr );
        if ( !_closeEvent )
        {
            result = HRESULT_FROM_WIN32(GetLastError());
        }

        if ( !_presenter )
        {
            _presenter = new EVRCustomPresenter(result);
            _presenter->SetVideoWindow ( _videoWindow );
        }

        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::CreateSession ( )
    {
        HRESULT result = CloseSession();
        if ( FAILED ( result ) ) return result;

        lucy_assert ( _state == PlayerState::Closed, "Player should be Closed!" );
        
        result = MFCreateMediaSession ( nullptr, &_session );
        if ( FAILED ( result ) ) return result;

        result = _session->BeginGetEvent ( this, nullptr );
        if ( FAILED ( result ) ) return result;

        _state = PlayerState::Ready;

        Volume(_volume);

        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::CloseSession ( )
    {
        HRESULT result = S_OK;

        if ( _videoDisplay != nullptr ) SafeRelease ( &_videoDisplay );
        if ( _volumeControl != nullptr ) SafeRelease ( &_volumeControl );

        if ( _session )
        {
            _state = PlayerState::Closing;
            result = _session->Close();

            if ( SUCCEEDED ( result ) )
            {
                DWORD waitResult = WaitForSingleObject( _closeEvent, 5000 );
                if ( waitResult == WAIT_TIMEOUT )
                {
                    lucy_assert ( false, "Timeout waiting for Close Event!" );
                    return -1;
                }
            }
        }

        if ( SUCCEEDED ( result ) )
        {
            if ( _source )
            {
                _source->Shutdown();
            }

            if ( _session )
            {
                _session->Shutdown();
            }
        }

        SafeRelease ( &_source );
        SafeRelease ( &_session );
        
        _state = PlayerState::Closed;
        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::StartPlayback ( )
    {
        lucy_assert ( _session != nullptr, "Attempted to start playback without a session!" );

        PROPVARIANT start;
        PropVariantInit(&start);

        HRESULT result = _session->Start ( &GUID_NULL, &start );

        if ( SUCCEEDED ( result ) )
        {
            _state = PlayerState::Started;
        }

        PropVariantClear ( &start );
        
        Volume(_volume);
        _isFinished = false;
        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::HandleEvent ( UINT_PTR eventPtr )
    {
        HRESULT status = S_OK;
        HRESULT result = S_OK;

        MediaEventType eventType = MEUnknown;
        IMFMediaEvent * mediaEvent = (IMFMediaEvent *)eventPtr;

        if ( mediaEvent == nullptr )
        {
            return E_POINTER;
        }

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            SafeRelease ( &mediaEvent );
            return r;
        };

        result = mediaEvent->GetType ( &eventType );
        if ( FAILED ( result ) )
        {
            return cleanup ( result );
        }

        result = mediaEvent->GetStatus ( &status );

        if ( SUCCEEDED ( result ) && FAILED ( status ) )
        {
            result = status;
        }

        if ( FAILED ( result ) )
        {
            return cleanup ( result );
        }

        switch ( eventType )
        {
            case MESessionTopologyStatus :
            {
                result = OnTopologyStatus ( mediaEvent );
                break;
            }

            case MEEndOfPresentation :
            {
                result = OnPresentationEnded ( mediaEvent );
                break;
            }

            case MENewPresentation :
            {
                result = OnNewPresentation ( mediaEvent );
                break;
            }

            case MESessionTopologySet :
            {
                IMFTopology * topology;
                
                // @TODO(andrew): Check for result of this?
                GetEventObject<IMFTopology> ( mediaEvent, &topology );
                
                WORD nodeCount = 0;
                topology->GetNodeCount ( &nodeCount );

                //Info ( Log::Msg() << "Topology has " << nodeCount << " nodes." );
                SafeRelease ( &topology );

                break;
            }

            case MESessionStarted :
            {
                Info ( "Session Started" );
                Volume(_volume);

                if (_state == PlayerState::Seeking)
                {
                    Info("Seek finished!");
                    _state = PlayerState::Started;
                    if (_afterSeekState == PlayerState::Stopped) Stop();
                    if (_afterSeekState == PlayerState::Paused) Pause();
                    _afterSeekState = PlayerState::Closed;
                }

                break;
            }

            case MEBufferingStarted :
            {
                Info ( "Buffering..." );
                break;
            }

            case MEBufferingStopped :
            {
                Info ( "Finished Buffering." );
                break;
            }

            case MEUpdatedStream:
            {
                Info("Stream Updated!");
                break;
            }

            /*case MESourceSeeked:
            {
                Info("Seeked!");
                break;
            }

            case MEStreamSeeked:
            {
                Info("Seeked!");
                break;
            }*/

            default :
            {
                result = OnSessionEvent ( mediaEvent, eventType );
                break;
            }
        }

        return cleanup ( result );
    }

    HRESULT WMFVideoPlayerPrivateImpl::SetMediaInfo ( IMFPresentationDescriptor * info )
    {
        _size.x = 0;
        _size.y = 0;

        HRESULT result = S_OK;
        GUID guidMajorType = GUID_NULL;

        IMFMediaTypeHandler * handler = nullptr;
        IMFStreamDescriptor * streamDesc = nullptr;
        IMFMediaType * sourceType = nullptr;

        auto cleanup = [&] ( HRESULT r ) -> HRESULT
        {
            SafeRelease ( &handler );
            SafeRelease ( &streamDesc );
            SafeRelease ( &sourceType );
            return r;
        };

        #define CHECK(hr) if ( FAILED ( hr ) ) return cleanup ( hr );

        DWORD count = 0;
        info->GetStreamDescriptorCount ( &count );

        for ( DWORD i = 0; i < count; i++ )
        {
            BOOL selected;
            result = info->GetStreamDescriptorByIndex ( i, &selected, &streamDesc );
            CHECK(result);

            if ( selected )
            {
                result = streamDesc->GetMediaTypeHandler ( &handler );
                CHECK ( result );

                result = handler->GetMajorType ( &guidMajorType );
                CHECK ( result );

                if ( guidMajorType == MFMediaType_Video )
                {
                    result = handler->GetCurrentMediaType ( &sourceType );
                    UINT32 w, h;

                    result = MFGetAttributeSize(sourceType, MF_MT_FRAME_SIZE, &w, &h);
                    if ( result == S_OK )
                    {
                        _size = ivec2(w, h);
                    }

                    {
                        UINT32 numerator = 0;
                        UINT32 denominator = 0;

                        MFGetAttributeRatio ( sourceType, MF_MT_FRAME_RATE, &numerator, &denominator );
                        if ( denominator != 0 )
                        {
                            _fps = static_cast<float>(numerator) / static_cast<float>(denominator);
                        }
                    }
                }
            }
        }

        #undef CHECK
        return cleanup ( result );
    }

    HRESULT WMFVideoPlayerPrivateImpl::OnTopologyStatus ( IMFMediaEvent * event )
    {
        UINT32 status;
        HRESULT result = event->GetUINT32 ( MF_EVENT_TOPOLOGY_STATUS, &status );

        if ( SUCCEEDED ( result ) && ( status == MF_TOPOSTATUS_READY ) )
        {
            SafeRelease ( &_videoDisplay );
            result = StartPlayback ( );
            result = Pause ( );
        }

        return result;
    }

    HRESULT WMFVideoPlayerPrivateImpl::OnPresentationEnded ( IMFMediaEvent * event )
    {
        HRESULT result = S_OK;

        if ( IsLooping )
        {
            /*_state = PlayerState::Started;

            PROPVARIANT start;
            PropVariantInit(&start);

            start.vt = VT_I8;
            float rate = PlaybackRate ( );
            if ( rate > 0 )
            {
                start.hVal.QuadPart = 0.0f;
            }

            result = _session->Start ( &GUID_NULL, &start );
            if ( FAILED ( result ) )
            {
                Error ( "Error looping video" );
            }

            PropVariantClear ( &start );*/
            //SeekToTime(0.0f);
            Stop();
            Play();
            std::cout << "LOOP!\n";
        }else
        {
            _state = PlayerState::Stopped;
            _isFinished = true;

            if (_onComplete) _onComplete();
            // @TODO(andrew): Notify public interface
        }

        return S_OK;
    }

    HRESULT WMFVideoPlayerPrivateImpl::OnNewPresentation ( IMFMediaEvent * event )
    {
        IMFPresentationDescriptor * descriptor = nullptr;
        IMFTopology * topology = nullptr;

        HRESULT result = S_OK;

        auto cleanup = [&] ( ) -> HRESULT
        {
            SafeRelease ( &descriptor );
            SafeRelease ( &topology );
            return S_OK;
        };

        #define CHECK(hr) if(FAILED(hr)) return cleanup ();

        result = GetEventObject ( event, &descriptor );
        CHECK ( result );

        result = CreatePlaybackTopology ( _source, descriptor, _videoWindow, &topology, _presenter );
        CHECK ( result );

        SetMediaInfo ( descriptor );

        result = _session->SetTopology ( 0, topology );
        CHECK ( result );

        _state = PlayerState::OpenPending;

        #undef CHECK 
        return cleanup ( );
    }

    HRESULT WMFVideoPlayerPrivateImpl::OnSessionEvent ( IMFMediaEvent * event, MediaEventType type )
    {
        return S_OK;
    }
}
