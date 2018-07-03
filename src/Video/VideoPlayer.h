//
//  IVideoPlayer.h
//  Snepo
//
//  Created by Andrew Wright on 14/03/2016.
//
//

#ifndef Snepo_VideoPlayer_h
#define Snepo_VideoPlayer_h

#include <memory>
#include <cstdint>

using u32 = std::uint32_t;

template <typename T>
using Observer = T *;

namespace Video
{
    using VideoPlayerRef = std::shared_ptr<class VideoPlayer>;
    class VideoPlayer
    {
    public:
        
        enum class                      ErrorCode
        {
            FileNotFound                = 0x1,
            UnsupportedCodec            = 0x2,
            UnknownError                = 0x3,
            InvalidSeek                 = 0x4,
            InvalidPlaybackMethod       = 0x5
        };
        
        enum class Capability
        {
            Seeking                     = 0x01,
            VariablePlayback            = 0x02,
            Reverse                     = 0x04
        };
        
        static std::string              kAudioDevice;
        
        using CompleteFn                = std::function<void(Observer<VideoPlayer>)>;
        using ErrorFn                   = std::function<void(Observer<const VideoPlayer>, ErrorCode)>;
        
        virtual ~VideoPlayer            ( );

        virtual void                	Init                ( );
        virtual void                    Update              ( );
        virtual void                    Draw                ( const ci::Rectf& bounds );
        virtual void                    DrawWithShader      ( const ci::Rectf& bounds, const ci::gl::GlslProgRef& shader );
        
        virtual void                    Play                ( const std::string& path );
        virtual void                    Stop                ( bool complete = false );
        
        virtual void                    Loops               ( bool loops );
        virtual bool                    Loops               ( ) const;
        
        virtual void                    BindTexture         ( );
        virtual void                    UnbindTexture       ( );
        
        virtual void                    Pause               ( );
        virtual void                    Resume              ( );
        
        virtual bool                    IsPlaying           ( ) const;
        virtual bool                    IsPaused            ( ) const;
        
        virtual bool                    SupportsCapability  ( Capability capability ) const;
        
        virtual ci::gl::Texture2dRef    Texture             ( ) const;
        
        virtual u32                     CurrentFrame        ( ) const;
        virtual u32                     TotalFrames         ( ) const;
        
        virtual ci::vec2                Size                ( ) const;
        virtual ci::Area                Bounds              ( ) const;
        
        virtual float            		Time                ( ) const;
        virtual float                   Duration            ( ) const;
        
        virtual float                   PercentComplete     ( ) const;
        
        virtual void                    SeekToFrame         ( u32 frame ) const;
        virtual void                    SeekToTime          ( float time ) const;
        
        virtual void                    Volume              ( float volume );
        virtual float                   Volume              ( ) const;
        
        virtual void                    PlayReversed        ( );
        virtual void                    PlaybackRate        ( float rate );
        virtual float                   PlaybackRate        ( ) const;
        
        virtual void                    OnComplete          ( CompleteFn handler );
        virtual void                    OnError             ( ErrorFn handler );
    
    protected:
        
        void                            NotifyComplete      ( );
        void                            NotifyError         ( ErrorCode code ) const;
        
        VideoPlayerRef                  Impl                ( ) const;

        mutable VideoPlayerRef          _impl;
        CompleteFn                      _completeFn;
        ErrorFn                         _errorFn;
        bool                            _loops{false};
    };
}

#endif /* Snepo_VideoPlayer_h */
