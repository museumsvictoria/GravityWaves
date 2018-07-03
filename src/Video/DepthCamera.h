//
//  DepthCamera.h
//  GravityWaves
//
//  Created by Andrew Wright on 27/11/17.
//

#ifndef DepthCamera_h
#define DepthCamera_h

#include <Common/Types.h>

namespace Video
{
    struct Format
    {
        float                       FPS{30.0f};
        bool                        RunInOwnThread{false};
    };
    
    struct DepthFrame
    {
        ci::Channel16uRef           FrameData;
        u32                         Frame;
    };
    
    using DepthCameraRef            = std::unique_ptr<class DepthCamera>;
    class DepthCamera               : public Serializable
    {
    public:
        
        using FrameSignal           = ci::signals::Signal<void(DepthFrame)>;
        
        DepthCamera                 ( const Format& format );
        virtual ~DepthCamera        ( ) { }
        
        bool                        Init            ( );
        void                        Shutdown        ( );
        
        bool                        Start           ( );
        void                        Stop            ( );

        void                        Write           ( ci::JsonTree& tree );
        void                        Read            ( const ci::JsonTree& tree );
        
        void                        Inspect         ( );
        
        virtual const char *        GetName         ( ) const = 0;
        
        inline bool                 IsStarted       ( ) const { return _isStarted; };
        inline bool                 IsInit          ( ) const { return _isInit; };
        inline const Format&        GetFormat       ( ) const { return _format; }
        
        FrameSignal                 OnFrame;
      
    protected:
        
        void                        NotifyFrame     ( DepthFrame frame, bool onMainThread = true );
        
        void                        Tick            ( );
        
        virtual bool                PrivateInit     ( ) { return false; };
        virtual void                PrivateTick     ( ) { };
        virtual void                PrivateInspect  ( ) { };
        virtual bool                PrivateStart    ( ) { return false; };
        virtual void                PrivateStop     ( ) { };
        virtual void                PrivateShutdown ( ) { };
        
        std::atomic<bool>           _isStarted{false};
        bool                        _isInit{false};
        Format                      _format;
        std::thread                 _runner;
        ci::signals::Connection     _tickConnection;
        std::atomic<u32>            _frameCount{0};
    };
    
    DepthCameraRef                  CreateDepthCamera ( const Format& format = Format() );
}

#endif /* DepthCamera_h */
