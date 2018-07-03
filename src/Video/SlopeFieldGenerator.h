//
//  SlopeFieldGenerator.h
//  GravityWaves
//
//  Created by Andrew Wright on 11/4/18.
//

#ifndef SlopeFieldGenerator_h
#define SlopeFieldGenerator_h

namespace Video
{
    using SlopeFieldGeneratorRef = std::unique_ptr<class SlopeFieldGenerator>;
    class SlopeFieldGenerator
    {
    public:
        
        SlopeFieldGenerator         ( const ci::ivec2& size );
        
        void                        Generate    ( const ci::gl::TextureRef& depth );
        ci::gl::TextureRef          GetSlope    ( ) const { return _buffer[_read]->getColorTexture(); };
        void                        Swap        ( ) { _read = 1 - _read; }
        
        const ci::ivec2&            Size        ( ) const { return _size; }
        ci::Area                    Bounds      ( ) const { return ci::Area ( ci::ivec2(0), _size ); }
        
        float                       MinDepth{0.021f};
        float                       MaxDepth{1.0f};
        float                       AccumRate{0.5f};
        float                       PeakThreshold{0.0195f};
        
    protected:
        
        ci::gl::GlslProgRef         _shader;
        ci::gl::FboRef              _buffer[2];
        ci::ivec2                   _size;
        int                         _read{0};
    };
}

#endif /* SlopeFieldGenerator_h */
