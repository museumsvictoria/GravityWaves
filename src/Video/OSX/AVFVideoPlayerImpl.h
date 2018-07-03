//
//  AVFVideoPlayerImpl.h
//  Snepo
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#ifndef Snepo_AVFVideoPlayerImpl_h
#define Snepo_AVFVideoPlayerImpl_h

#include <Video/VideoPlayer.h>

namespace cinder
{
    namespace qtime
    {
        using MovieGlFwdRef = std::shared_ptr<class MovieGl>;
    }
}

namespace Video
{
    class AVFVideoPlayerImpl : public VideoPlayer
    {
    public:
        
        void                        Init                ( ) override;
        void                        Update              ( ) override;
        void                        Draw                ( const ci::Rectf& bounds ) override;
        
        void                        Play                ( const std::string& path ) override;
        void                        Stop                ( bool complete = false ) override;
        
        void                        Loops               ( bool loops ) override;
        bool                        Loops               ( ) const override;
        
        void                        Pause               ( ) override;
        void                        Resume              ( ) override;
        
        bool                        IsPlaying           ( ) const override;
        bool                        IsPaused            ( ) const override;
        
        bool                        SupportsCapability  ( Capability capability ) const override;
        
        ci::gl::Texture2dRef        Texture             ( ) const override;
        
        u32                         CurrentFrame        ( ) const override;
        u32                         TotalFrames         ( ) const override;
        
        ci::vec2                    Size                ( ) const override;
        ci::Area                    Bounds              ( ) const override;
        
        float                       Time                ( ) const override;
        float                       Duration            ( ) const override;
        
        float                       PercentComplete     ( ) const override;
        
        void                        SeekToFrame         ( u32 frame ) const override;
        void                        SeekToTime          ( float time ) const override;
        
        void                        PlayReversed        ( ) override;
        void                        PlaybackRate        ( float rate ) override;
        float                       PlaybackRate        ( ) const override;
        
        void                        Volume              ( float volume ) override;
        float                       Volume              ( ) const override;
        
        void                        OnComplete          ( CompleteFn handler ) override;
        void                        OnError             ( ErrorFn handler ) override;
        
    protected:
        
        ci::qtime::MovieGlFwdRef    _player;
        ci::gl::Texture2dRef        _texture;
        float                       _rate{1.0f};
        bool                        _isPlaying{false};
        bool                        _loops{false};
    };
}


#endif /* Snepo_AVFVideoPlayerImpl_h */
