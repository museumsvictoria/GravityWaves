//
//  DepthCamera.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 27/11/17.
//

#include <Video/DepthCamera.h>
#include "CinderImGui.h"

#ifdef CINDER_COCOA
    #include <Video/OSX/ImageSequenceDepthCamera.h>
    using CameraType = Video::ImageSequenceDepthCamera;
#else
    #include <Video/Win32/KinectDepthCamera.h>
    using CameraType = Video::KinectDepthCamera;
#endif

using namespace ci;

namespace Video
{
    DepthCameraRef CreateDepthCamera ( const Format& format )
    {
        auto camera = std::make_unique<CameraType> ( format );
        camera->Init();
        return camera;
    }
    
    DepthCamera::DepthCamera ( const Format& format )
    : Serializable( "DepthCamera" )
    , _format ( format )
    { }
    
    bool DepthCamera::Init ( )
    {
        if ( _isInit ) return false;
        return ( _isInit = PrivateInit() );
    }
    
    void DepthCamera::Shutdown ( )
    {
        Stop();
        PrivateShutdown();
    }
    
    void DepthCamera::Tick ( )
    {
        #ifndef NDEBUG
        if ( GetFormat().RunInOwnThread && app::isMainThread() )
        {
            assert ( false && "Cannot call Tick() manually when DepthCamera is multithreaded!" );
        }
        #endif
        
        PrivateTick();
    }
    
    void DepthCamera::Inspect ( )
    {
        if ( ui::CollapsingHeader ( "Depth Camera" ) )
        {
            ui::Text ( "Name: %s | Frame: %d | Started: %s", GetName(), _frameCount.load(), _isStarted ? "true" : "false" );
            PrivateInspect();
        }
    }
    
    void DepthCamera::Write ( ci::JsonTree& tree )
    {
        
    }
    
    void DepthCamera::Read ( const ci::JsonTree& tree )
    {
        
    }
    
    void DepthCamera::NotifyFrame ( DepthFrame frame, bool onMainThread )
    {
        frame.Frame = _frameCount++;

        if ( onMainThread && !app::isMainThread() )
        {
            app::App::get()->dispatchAsync( [=]
            {
                OnFrame.emit ( frame );
            } );
        }else
        {
            OnFrame.emit ( frame );
        }
    }
    
    bool DepthCamera::Start ( )
    {
        if ( IsStarted() ) return false;
        if ( !IsInit() ) return false;
        
        _isStarted = PrivateStart();
        
        if ( GetFormat().RunInOwnThread )
        {
            _runner = std::thread ( [=]
            {
                while ( _isStarted )
                {
                    Tick();
                }
            } );
        }else
        {
            _tickConnection = app::App::get()->getSignalUpdate().connect ( [=] { Tick(); } );
        }
        return _isStarted;
    }
    
    void DepthCamera::Stop ( )
    {
        if ( !IsStarted() ) return;
        
        _isStarted = false;
        
        if ( GetFormat().RunInOwnThread )
        {
            if ( _runner.joinable() )
            {
                _runner.join();
            }
        }else
        {
            _tickConnection.disable();
            _tickConnection.disconnect();
        }
        
        PrivateStop();
    }
}
