//
//  ImageSequenceDepthCamera.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 27/11/17.
//

#include <Video/OSX/ImageSequenceDepthCamera.h>
#include "CinderImGui.h"

using namespace ci;
namespace Video
{
    ImageSequenceDepthCamera::ImageSequenceDepthCamera ( const Format& format )
    : DepthCamera ( format )
    { }
    
    bool ImageSequenceDepthCamera::PrivateInit ( )
    {
        {
            auto folder = app::getAssetPath ( "Frames" );
            if ( !folder.empty() )
            {
                fs::directory_iterator it { folder }, end;
                while ( it != end )
                {
                    if ( it->path().extension().string() == ".exr" )
                    {
                        OfflineFrame frame;
                        frame.Path = it->path().string();
                        _frames.push_back ( frame );
                    }
                    
                    if ( _frames.size() > 128 ) break;
                    it++;
                }
            }
        }
        
        return !_frames.empty();
    }
    
    void ImageSequenceDepthCamera::PrivateInspect ( )
    {
        ui::DragInt ( "Clip Size", &_border, 0, 0, 64 );
    }
    
    void ImageSequenceDepthCamera::PrivateTick ( )
    {
        int tick = 60.0f / _format.FPS;
        
        if ( app::getElapsedFrames() % tick == 0 )
        {
            _frameIndex += _tick;
            _frameIndex %= _frames.size();
            
            try
            {
                DepthFrame d;
                
                auto& frame = _frames[_frameIndex];
                if ( frame.Channel )
                {
                    d.FrameData = frame.Channel;
                }else
                {
                    int border = _border;
                    
                    auto channel = Channel16u::create ( loadImage ( loadFile ( frame.Path ) ) );
                    if ( _border > 0)
                    {
                        auto bounds = channel->getBounds();
                        bounds.x1 += border; bounds.x2 -= border;
                        bounds.y1 += border; bounds.y2 -= border;
                        
                        d.FrameData = Channel16u::create ( channel->clone( bounds, true ) );
                        frame.Channel = d.FrameData;
                    }else
                    {
                        d.FrameData = channel;
                        frame.Channel = channel;
                    }
                }
                
                NotifyFrame( d );
                
            }catch ( ... )
            {
                _frameIndex = 0;
            }
        }
    }
    
    bool ImageSequenceDepthCamera::PrivateStart ( )
    {
        _tick = 1;
        return true;
    }
    
    void ImageSequenceDepthCamera::PrivateStop ( )
    {
        _tick = 0;
    }
    
    void ImageSequenceDepthCamera::PrivateShutdown ( )
    {
        _tick = 0;
    }
}
