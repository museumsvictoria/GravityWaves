//
//  AVFVideoPlayerImpl.cxx
//  Snepo
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#include <Video/OSX/AVFVideoPlayerImpl.h>
#include "cinder/qtime/QuickTimeGl.h"

using namespace ci;

namespace Video
{
    void AVFVideoPlayerImpl::Init ( )
    {
        
    }
    
    void AVFVideoPlayerImpl::Update ( )
    {
        if ( _player )
        {
            if ( _player->checkNewFrame() )
            {
                _texture = _player->getTexture();
            }
        }
    }
    
    void AVFVideoPlayerImpl::Loops ( bool loops )
    {
        _loops = loops;
        if ( _player ) _player->setLoop(loops);
    }
    
    bool AVFVideoPlayerImpl::Loops ( ) const
    {
        return _loops;
    }
    
    void AVFVideoPlayerImpl::Draw ( const ci::Rectf& bounds )
    {
        if ( _texture )
        {
            gl::draw ( _texture, bounds );
        }
    }
    
    void AVFVideoPlayerImpl::Play ( const std::string& path )
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
            _player = qtime::MovieGl::create( fullPath );
            _player->setVolume(0.0f);
            _player->setLoop(_loops);
            _player->play();
            _player->getEndedSignal().connect ( std::bind ( &AVFVideoPlayerImpl::NotifyComplete, this ) );
            _isPlaying = true;
            
        }catch ( const qtime::AvfPathInvalidExc& )
        {
            NotifyError( ErrorCode::FileNotFound );
        }catch ( const qtime::AvfErrorLoadingExc& )
        {
            NotifyError( ErrorCode::UnsupportedCodec );
        }catch ( ... )
        {
            NotifyError( ErrorCode::UnknownError );
        }
    }
    
    void AVFVideoPlayerImpl::Stop ( bool complete )
    {
        if ( IsPlaying() && _player )
        {
            _isPlaying = false;
            _player->stop();
        }
    }
    
    void AVFVideoPlayerImpl::Pause ( )
    {
        if ( IsPlaying() && _player )
        {
            _isPlaying = false;
            _player->play(false);
        }
    }
    
    void AVFVideoPlayerImpl::Resume ( )
    {
        if ( IsPaused() && _player )
        {
            _isPlaying = true;
            _player->play();
        }
    }
    
    bool AVFVideoPlayerImpl::IsPlaying ( ) const
    {
        return _isPlaying;
    }
    
    bool AVFVideoPlayerImpl::IsPaused ( ) const
    {
        return !IsPlaying();
    }
    
    bool AVFVideoPlayerImpl::SupportsCapability ( Capability capability ) const
    {
        return true;
    }
    
    gl::Texture2dRef AVFVideoPlayerImpl::Texture ( ) const
    {
        return _texture;
    }
    
    u32 AVFVideoPlayerImpl::CurrentFrame ( ) const
    {
        if ( _player ) return _player->getCurrentTime() * _player->getFramerate();
        return 0;
    }
    
    u32 AVFVideoPlayerImpl::TotalFrames ( ) const
    {
        if ( _player ) return _player->getDuration() * _player->getFramerate();
        return 0;
    }
    
    vec2 AVFVideoPlayerImpl::Size ( ) const
    {
        if ( _player ) return _player->getSize();
        return vec2(0);
    }
    
    Area AVFVideoPlayerImpl::Bounds ( ) const
    {
        if ( _player ) return _player->getBounds();
        return Area(0, 0, 0, 0);
    }
    
    float AVFVideoPlayerImpl::Time ( ) const
    {
        if ( _player ) return _player->getCurrentTime();
        return 0;
    }
    
    float AVFVideoPlayerImpl::Duration ( ) const
    {
        if ( _player ) return _player->getDuration();
        return 0;
    }
    
    float AVFVideoPlayerImpl::PercentComplete ( ) const
    {
        if ( Duration() != 0 )
            return Time() / Duration();
        
        return 0;
    }
    
    void AVFVideoPlayerImpl::SeekToFrame ( u32 frame ) const
    {
        if ( _player ) _player->seekToFrame( frame );
    }
    
    void AVFVideoPlayerImpl::SeekToTime ( float time ) const
    {
        if ( _player ) _player->seekToTime( time );
    }
    
    void AVFVideoPlayerImpl::PlayReversed ( )
    {
        PlaybackRate ( -1.0f );
    }
    
    void AVFVideoPlayerImpl::PlaybackRate ( float rate )
    {
        if ( _player ) _player->setRate( rate );
        _rate = rate;
    }
    
    float AVFVideoPlayerImpl::PlaybackRate ( ) const
    {
        if ( _player ) return _rate;
        return 1.0f;
    }
    
    void AVFVideoPlayerImpl::Volume ( float volume )
    {
        if ( _player ) _player->setVolume ( volume );
    }
    
    float AVFVideoPlayerImpl::Volume ( ) const
    {
        if ( _player ) return _player->getVolume();
        return 0.0f;
    }
    
    void AVFVideoPlayerImpl::OnComplete ( CompleteFn handler )
    {
        _completeFn = handler;
    }
    
    void AVFVideoPlayerImpl::OnError ( ErrorFn handler )
    {
        _errorFn = handler;
    }
}
