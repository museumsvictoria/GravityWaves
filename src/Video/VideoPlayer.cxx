//
//  VideoPlayer.cxx
//  Snepo
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#include <Video/VideoPlayer.h>

using namespace ci;

namespace Video
{
    std::string VideoPlayer::kAudioDevice = "";
    
    void VideoPlayer::Init ( )
    {
        Impl()->Init ( );
    }
    
    void VideoPlayer::Update ( )
    {
        Impl()->Update ( );
    }
    
    void VideoPlayer::Draw ( const ci::Rectf& bounds )
    {
        Impl()->Draw ( bounds );
    }
    
    void VideoPlayer::DrawWithShader ( const ci::Rectf& bounds, const ci::gl::GlslProgRef& shader )
    {
        Impl()->DrawWithShader( bounds, shader );
    }
    
    void VideoPlayer::Play ( const std::string& path )
    {
        Impl()->Play ( path );
    }
    
    void VideoPlayer::Stop ( bool complete )
    {
        Impl()->Stop ( complete );
        if ( complete ) NotifyComplete();
    }
    
    void VideoPlayer::Loops ( bool loops )
    {
        Impl()->Loops ( loops );
    }
    
    bool VideoPlayer::Loops ( ) const
    {
        return Impl()->Loops();
    }
    
    void VideoPlayer::BindTexture ( )
    {
        Impl()->BindTexture();
    }
    
    void VideoPlayer::UnbindTexture ( )
    {
        Impl()->UnbindTexture();
    }
    
    void VideoPlayer::Pause ( )
    {
        Impl()->Pause ( );
    }
    
    void VideoPlayer::Resume ( )
    {
        Impl()->Resume ( );
    }
    
    bool VideoPlayer::IsPlaying ( ) const
    {
        return Impl()->IsPlaying ( );
    }
    
    bool VideoPlayer::IsPaused ( ) const
    {
        return Impl()->IsPaused ( );
    }
    
    bool VideoPlayer::SupportsCapability ( Capability capability ) const
    {
        return Impl()->SupportsCapability( capability );
    }

    gl::Texture2dRef VideoPlayer::Texture ( ) const
    {
        return Impl()->Texture();
    }
    
    u32 VideoPlayer::CurrentFrame ( ) const
    {
        return Impl()->CurrentFrame ( );
    }
    
    u32 VideoPlayer::TotalFrames ( ) const
    {
        return Impl()->TotalFrames();
    }
    
    vec2 VideoPlayer::Size ( ) const
    {
        return Impl()->Size();
    }
    
    Area VideoPlayer::Bounds ( ) const
    {
        return Impl()->Bounds();
    }
    
    float VideoPlayer::Time ( ) const
    {
        return Impl()->Time();
    }
    
    float VideoPlayer::Duration ( ) const
    {
        return Impl()->Duration();
    }
    
    float VideoPlayer::PercentComplete ( ) const
    {
        return Impl()->PercentComplete();
    }
    
    void VideoPlayer::SeekToFrame ( u32 frame ) const
    {
        if ( !SupportsCapability( Capability::Seeking ) )
        {
            NotifyError ( ErrorCode::InvalidSeek );
            return;
        }
        
        Impl()->SeekToFrame( frame );
    }
    
    void VideoPlayer::SeekToTime ( float time ) const
    {
        if ( !SupportsCapability( Capability::Seeking ) )
        {
            NotifyError ( ErrorCode::InvalidSeek );
            return;
        }
        
        Impl()->SeekToTime( time );
    }
    
    void VideoPlayer::PlayReversed ( )
    {
        if ( !SupportsCapability( Capability::Reverse ) )
        {
            NotifyError ( ErrorCode::InvalidPlaybackMethod );
            return;
        }
        
        Impl()->PlayReversed ( );
    }
    
    void VideoPlayer::PlaybackRate ( float rate )
    {
        if ( !SupportsCapability( Capability::VariablePlayback ) )
        {
            NotifyError ( ErrorCode::InvalidPlaybackMethod );
            return;
        }
        
        Impl()->PlaybackRate( rate );
    }
    
    float VideoPlayer::PlaybackRate ( ) const
    {
        if ( !SupportsCapability( Capability::VariablePlayback ) )
        {
            return 1.0f;
        }
        
        return Impl()->PlaybackRate();
    }
    
    void VideoPlayer::Volume ( float volume )
    {
        Impl()->Volume( volume );
    }
    
    float VideoPlayer::Volume ( ) const
    {
        return Impl()->Volume();
    }
    
    void VideoPlayer::OnComplete ( CompleteFn handler )
    {
        _completeFn = handler;
        Impl()->OnComplete( handler );
    }
    
    void VideoPlayer::OnError ( ErrorFn handler )
    {
        _errorFn = handler;
        Impl()->OnError( handler );
    }
    
    void VideoPlayer::NotifyComplete ( )
    {
        if ( _completeFn )
        {
            _completeFn ( this );
        }
    }
    
    void VideoPlayer::NotifyError ( ErrorCode code ) const
    {
        if ( _errorFn )
        {
            _errorFn ( this, code );
        }
    }

    VideoPlayer::~VideoPlayer ( )
    {

    }
}

#ifdef CINDER_COCOA

#include <Video/OSX/AVFVideoPlayerImpl.h>

Video::VideoPlayerRef Video::VideoPlayer::Impl ( ) const
{
    if ( !_impl )
    {
        _impl = std::make_shared<Video::AVFVideoPlayerImpl>( /*kAudioDevice*/ );
        _impl->Init();
    }
    
    return _impl;
}

#else

#define FORCE_USE_WMF_VIDEO
#if defined(FORCE_USE_WMF_VIDEO)

#include <Video/Win32/WMF/WMFVideoPlayerImpl.h>

Video::VideoPlayerRef Video::VideoPlayer::Impl ( ) const
{
	if ( !_impl ) 
	{
		_impl = std::make_shared<Video::WMFVideoPlayerImpl>( kAudioDevice );
		_impl->Init();
	}
    return _impl;
}

#else

#error "Implement QuickTime software rendered fallback!"

#endif

#endif


