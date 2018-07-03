//
//  ImageSequenceDepthCamera.h
//  GravityWaves
//
//  Created by Andrew Wright on 27/11/17.
//

#ifndef ImageSequenceDepthCamera_h
#define ImageSequenceDepthCamera_h

#include <Video/DepthCamera.h>

namespace Video
{
    class ImageSequenceDepthCamera : public DepthCamera
    {
    public:
        
        ImageSequenceDepthCamera    ( const Format& format );
        
        const char *                GetName         ( ) const override { return "ImageSequenceDepthCamera"; }
        
    protected:
        
        void                        PrivateInspect  ( ) override;
        
        bool                        PrivateInit     ( ) override;
        void                        PrivateTick     ( ) override;
        bool                        PrivateStart    ( ) override;
        void                        PrivateStop     ( ) override;
        void                        PrivateShutdown ( ) override;
        
        struct OfflineFrame
        {
            std::string             Path;
            ci::Channel16uRef       Channel{nullptr};
        };
        
        u32                         _tick{0};
        u32                         _frameIndex{0};
        std::vector<OfflineFrame>   _frames;
        s32                         _border{0};
    };
}

#endif /* ImageSequenceDepthCamera_hpp */
