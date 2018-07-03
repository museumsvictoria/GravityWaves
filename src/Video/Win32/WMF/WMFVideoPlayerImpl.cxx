//
//  WMFVideoPlayerImpl.cxx
//  SFMoma
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#include <Video/Win32/WMF/WMFVideoPlayerImpl.h>
#include <Video/Win32/WMF/WMFVideoPlayerPrivateImpl.h>
#include "cinder/gl/gl.h"

using namespace ci;

namespace Video
{
    using WMFPlayer                     = Observer<WMFVideoPlayerImpl>;    
}

namespace
{
    using WindowToPlayer                = std::pair<HWND, Video::WMFPlayer>;

    static bool                         kRequiresGlobalInit = true; 
	static u32                          kActiveInstances = 0;
    static std::vector<WindowToPlayer>  kAllInstances;

#ifndef NDEBUG
#define lucy_assert(x, y) assert(x)
#else
#define lucy_assert(x, y)
#endif

    static bool GlobalInit ( )
    {
        MFStartup(MF_VERSION);

        kRequiresGlobalInit = false;
        return true;
    }

    static bool GlobalShutdown ( )
    {   
        MFShutdown();
        kRequiresGlobalInit = true;
        return true;
    }

    Video::WMFPlayer FindPlayerByWindow ( HWND window )
    {
        for ( auto& player : kAllInstances )
        {
            if ( player.first == window ) return player.second;
        }

        return nullptr;
    }

    LRESULT CALLBACK InterceptedWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
		    case WM_CREATE:
		    {
			    return DefWindowProc ( hwnd, message, wParam, lParam );
		    }
		    default:
		    {
			    auto player = FindPlayerByWindow ( hwnd );
			    if ( !player )
                {
				    return DefWindowProc(hwnd, message, wParam, lParam);
                }else
                {
			        return player->WndProc (hwnd, message, wParam, lParam);
                }
		    }
        }
        return 0;
    }
}

namespace Video 
{
    WMFVideoPlayerImpl::WMFVideoPlayerImpl ( const std::string& audioDevice )
    : _player(nullptr)
    , _audioDevice(audioDevice)
    {
        if ( kRequiresGlobalInit ) GlobalInit ( );

        _id = kActiveInstances++;
		if (!InitInstance())
		{
			Error("Error initializing instance!");
		}
    }

    void WMFVideoPlayerImpl::Init ( )
    {
        Info ( "Using Audio Device: " + VideoPlayer::kAudioDevice );
    }
    
    void WMFVideoPlayerImpl::Update ( )
    {
        if ( !_player ) return;
        if ( ( _waitForLoadedToPlay ) && _player->GetState() == PlayerState::Paused )
        {
            _waitForLoadedToPlay = false;
            _player->Play ( );
            _isPlaying = true;
        }

		//Info(Log::Msg() << Time() << "/" << Duration());
    }

	
	void WMFVideoPlayerImpl::Loops ( bool loops )
	{
		if (_player)
		{
			_loops = loops;
			_player->IsLooping = loops;
		}
	}

	void WMFVideoPlayerImpl::BindTexture()
	{
		if (_texture)
		{
			_player->Presenter()->LockSharedTexture();
			_texture->bind();
		}
	}

	void WMFVideoPlayerImpl::UnbindTexture()
	{
		if (_texture)
		{
			_player->Presenter()->UnlockSharedTexture();
			_texture->unbind();
		}
	}

    
    void WMFVideoPlayerImpl::Draw ( const ci::Rectf& bounds )
    {
		if ( _texture )
        {
			Rectf r = bounds;
            std::swap ( r.y1, r.y2 );
            _player->Presenter()->LockSharedTexture();

			gl::draw(_texture, r );
            _player->Presenter()->UnlockSharedTexture();
		}
    }

    void WMFVideoPlayerImpl::DrawWithShader ( const ci::Rectf& bounds, const ci::gl::GlslProgRef& shader )
    {
        if ( _texture && shader )
        {
            _player->Presenter()->LockSharedTexture();
            gl::ScopedGlslProg glsl ( shader );
            gl::ScopedTextureBind tex0 ( _texture, 0 );
            gl::drawSolidRect( bounds, vec2(0), bounds.getSize() );
            _player->Presenter()->UnlockSharedTexture();
        }else
        {
            Draw ( bounds );
        }
    }
    
    void WMFVideoPlayerImpl::Play ( const std::string& path )
    {
        if ( !fs::exists( path ) && !fs::exists ( app::getAssetPath( path ) ) )
        {
            NotifyError( ErrorCode::FileNotFound );
            return;
        }
        
        std::string fullPath = path;
        if ( !fs::exists ( path ) )
        {
            fullPath = app::getAssetPath( path ).string();
        }
        
        try
        {
            HRESULT result = S_OK;
	        std::wstring w(fullPath.length(), L' ');
	        std::copy(fullPath.begin(), fullPath.end(), w.begin());

	        std::wstring a(_audioDevice.length(), L' ');
	        std::copy(_audioDevice.begin(), _audioDevice.end(), a.begin());

	        result = _player->OpenURL( w.c_str(), a.c_str() );
			_player->Volume(_volume);

	        if (!_sharedTextureCreated)
	        {
		        _size.x = _player->Width();
		        _size.y = _player->Height();
	
				//Info(Log::Msg() << "Creating initial shared texture: " << _size);

		        gl::Texture::Format fmt = gl::Texture::Format().internalFormat(GL_RGBA).target ( GL_TEXTURE_RECTANGLE );
		        _texture = gl::Texture::create ( _size.x, _size.y, fmt );
		        
				if (!_player->Presenter()->CreateSharedTexture(_size.x, _size.y, _texture->getId()))
				{
					Error("Error Creeating Shared Texture!");
				}
		        _sharedTextureCreated = true;
	        }else 
	        {
		        if ( ( _size.x != _player->Width() ) || ( _size.y != _player->Height() ) )
		        {
					_player->Presenter()->ReleaseSharedTexture();

			        _size.x = _player->Width();
			        _size.y = _player->Height();

					//Info(Log::Msg() << "Replacing shared texture: " << _size);

					gl::Texture::Format fmt = gl::Texture::Format().internalFormat(GL_RGBA).target ( GL_TEXTURE_RECTANGLE );
		            _texture = gl::Texture::create ( _size.x, _size.y, fmt );

					if (!_player->Presenter()->CreateSharedTexture(_size.x, _size.y, _texture->getId()))
					{
						Error("Error Creating Shared Texture!");
					}
		        }
	        }

	        _waitForLoadedToPlay = false;

        }catch ( const std::exception& e )
        {
			Error( e.what() );
            NotifyError( ErrorCode::UnknownError );
        }

        if ( _player->GetState() == PlayerState::OpenPending )
        {
            _waitForLoadedToPlay = true;
        }
         
        _isPlaying = true;
        _player->Play ( );
    }
    
    void WMFVideoPlayerImpl::Stop ( bool complete )
    {
        if ( IsPlaying() && _player )
        {
            _player->Stop();
            _isPlaying = false;
        }
    }
    
    void WMFVideoPlayerImpl::Pause ( )
    {
        if ( IsPlaying() && _player )
        {
            _player->Pause();
            _isPlaying = false;
        }
    }
    
    void WMFVideoPlayerImpl::Resume ( )
    {
        if ( IsPaused() && _player )
        {
            _isPlaying = true;
            _player->Play();
        }
    }
    
    bool WMFVideoPlayerImpl::IsPlaying ( ) const
    {
        return _isPlaying;
    }
    
    bool WMFVideoPlayerImpl::IsPaused ( ) const
    {
        return !IsPlaying();
    }
    
    bool WMFVideoPlayerImpl::SupportsCapability ( Capability capability ) const
    {
        return true;
    }
    
    gl::Texture2dRef WMFVideoPlayerImpl::Texture ( ) const
    {
        return _texture;
    }
    
    u32 WMFVideoPlayerImpl::CurrentFrame ( ) const
    {
        if ( _player ) return _player->Time() * _player->FPS();
        return 0;
    }
    
    u32 WMFVideoPlayerImpl::TotalFrames ( ) const
    {
        if ( _player ) return _player->Duration() * _player->FPS();
        return 0;
    }
    
    vec2 WMFVideoPlayerImpl::Size ( ) const
    {
        if ( _player ) return vec2(_player->Width(), _player->Height());
        return vec2(0);
    }
    
    Area WMFVideoPlayerImpl::Bounds ( ) const
    {
        if ( _player ) return Area(vec2(0), Size());
        return Area(0, 0, 0, 0);
    }
    
    float WMFVideoPlayerImpl::Time ( ) const
    {
        if ( _player ) return _player->Time();
        return 0;
    }
    
    float WMFVideoPlayerImpl::Duration ( ) const
    {
        if ( _player ) return _player->Duration();
        return 0;
    }
    
    float WMFVideoPlayerImpl::PercentComplete ( ) const
    {
        if ( Duration() != 0 )
            return Time() / Duration();
        
        return 0;
    }
    
    void WMFVideoPlayerImpl::SeekToFrame ( u32 frame ) const
    {
        if ( _player ) 
        {
            float ratio = static_cast<float> ( frame ) / static_cast<float> ( TotalFrames() );
            SeekToTime ( ratio * _player->Duration() );
        }
    }
    
    void WMFVideoPlayerImpl::SeekToTime ( float time ) const
    {
        if ( _player ) 
        {
            if ( !_player->SeekToTime( time ) )
            {
                NotifyError ( ErrorCode::InvalidSeek );
            }
        }
    }
    
    void WMFVideoPlayerImpl::PlayReversed ( )
    {
        PlaybackRate ( -1.0f );
    }
    
    void WMFVideoPlayerImpl::PlaybackRate ( float rate )
    {
        if ( _player ) _player->PlaybackRate( rate );
        _rate = rate;
    }
    
    float WMFVideoPlayerImpl::PlaybackRate ( ) const
    {
        if ( _player ) return _player->PlaybackRate();
        return 1.0f;
    }
    
    void WMFVideoPlayerImpl::Volume ( float volume )
    {
		_volume = volume;
        if ( _player ) _player->Volume ( volume );
    }
    
    float WMFVideoPlayerImpl::Volume ( ) const
    {
		if ( _player ) return _player->Volume();
        return _volume;
    }
    
    void WMFVideoPlayerImpl::OnComplete ( CompleteFn handler )
    {
        _completeFn = handler;
		_player->OnComplete([this] { NotifyComplete(); });
    }
    
    void WMFVideoPlayerImpl::OnError ( ErrorFn handler )
    {
        _errorFn = handler;
    }

    HWND WMFVideoPlayerImpl::GetPlayerWindow ( ) const
    {
        return _playerWindow;
    }
        
	LRESULT	WMFVideoPlayerImpl::WndProc ( HWND window, UINT message, WPARAM wParam, LPARAM lParam )
    {
        switch ( message )
        {
            case WM_DESTROY :
            {
                PostQuitMessage(0);
                break;
            }

            case WM_APP_PLAYER_EVENT :
            {
                OnPlayerEvent(window, wParam);
                break;
            }
            
            default:
                return DefWindowProc(window, message, wParam, lParam);
        }

        return 0;
    }

	bool WMFVideoPlayerImpl::InitInstance ( )
    {
        PCWSTR windowClassName = L"WMFVideoPlayerImpl";
        HWND window;
        
		static bool kHasRegistered = false;

		if (!kHasRegistered)
		{
			kHasRegistered = true;

			WNDCLASSEX windowClass;

			ZeroMemory(&windowClass, sizeof(WNDCLASSEX));

			windowClass.cbSize = sizeof(WNDCLASSEX);
			windowClass.style = CS_HREDRAW | CS_VREDRAW;
			windowClass.lpfnWndProc = InterceptedWndProc;
			windowClass.hbrBackground = (HBRUSH)BLACK_BRUSH;
			windowClass.lpszClassName = windowClassName;
			windowClass.hInstance = GetModuleHandle(NULL); 
	
			auto result = RegisterClassEx(&windowClass);
			
			char w[32] = { 0 }; 
			GetAtomNameA(result, w, 31);
			
			//Info(Log::Msg() << "Registering WindowClass: " << std::hex << result << std::dec << ": " << w );

			if ( result == 0 )
			{
				auto d = GetLastError();
				//Warn(Log::Msg() << "Error registering window class: " << d << " : " << std::hex << d << std::dec);
			}
		}
		
        window = CreateWindow(windowClassName, L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, nullptr, nullptr );
        if ( !window )
        {
            Error ( "Error creating overlapped window" );
            return false;
        }

        kAllInstances.emplace_back ( window, this );

        HRESULT result = WMFVideoPlayerPrivateImpl::CreateInstance ( window, window, &_player);
        
        LONG style = GetWindowLong(window, GWL_STYLE);
        style &= ~WS_DLGFRAME;
        style &= ~WS_CAPTION;
        style &= ~WS_BORDER;
        style &= ~WS_POPUP; // @TODO(andrew): should this be complement?

        LONG exStyle = GetWindowLong(window, GWL_EXSTYLE);
        exStyle &= ~WS_EX_DLGMODALFRAME;

        SetWindowLong (window, GWL_STYLE, style );
        SetWindowLong (window, GWL_EXSTYLE, exStyle );

        _playerWindow = window;
        UpdateWindow(window);

        return true;
    }

    void WMFVideoPlayerImpl::OnPlayerEvent( HWND window, WPARAM param )
    {
        HRESULT hr = _player->HandleEvent ( param );
        if ( FAILED(hr) )
        {
            Error ( "Error handling PlayerEvent!");
        }
    }

    WMFVideoPlayerImpl::~WMFVideoPlayerImpl ( )
    {
        if ( _player )
        {
            _isPlaying = false;
            _player->Shutdown();
            SafeRelease ( &_player );
        }

        if ( --kActiveInstances == 0 )
        {
            GlobalShutdown();
        }
    }
}
