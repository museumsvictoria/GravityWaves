//
//  SlopeFieldGenerator.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 11/4/18.
//

#include "SlopeFieldGenerator.h"

using namespace ci;

namespace Video
{
    SlopeFieldGenerator::SlopeFieldGenerator ( const ivec2& size )
    {
#ifdef CINDER_MAC
		MinDepth = 0.229f;
        PeakThreshold = 0.2279f;;
#else
#endif
        auto makeBuffer = [size]
        {
            auto tFmt = gl::Texture::Format().internalFormat(GL_RGB).minFilter(GL_LINEAR).magFilter(GL_LINEAR);
            auto slope = gl::Texture::create( size.x, size.y, tFmt );
            auto peaks = gl::Texture::create( size.x, size.y, tFmt );
            
            auto fmt = gl::Fbo::Format().samples(1).colorTexture ( tFmt );
            
            return gl::Fbo::create( size.x, size.y, fmt );
        };
        
        _size   = size;
        
        _buffer[0] = makeBuffer();
        _buffer[1] = makeBuffer();
        
        {
            auto fmt = gl::GlslProg::Format().vertex(app::loadAsset( "Shaders/Passthrough.vs.glsl" )).fragment(app::loadAsset( "Shaders/SlopeGenerator.fs.glsl" ));
            
            _shader = gl::GlslProg::create( fmt );
        }
    }

    void SlopeFieldGenerator::Generate ( const gl::TextureRef& depth )
    {
        _shader->uniform ( "uMinDepth", MinDepth );
        _shader->uniform ( "uMaxDepth", MaxDepth );
        _shader->uniform ( "uPeakThreshold", PeakThreshold );
        
        gl::ScopedMatrices mat;
        gl::ScopedViewport vp { ivec2(0), _size };
        
        gl::ScopedFramebuffer buffer { _buffer[1-_read] };
        gl::ScopedGlslProg shader { _shader };
        gl::ScopedTextureBind tex0 ( depth, 0 );
        
        gl::setMatricesWindow( _size );
        gl::clear( Colorf ( 0, 0, 0 ) );
        
        gl::drawSolidRect( Bounds() );
    }
}

