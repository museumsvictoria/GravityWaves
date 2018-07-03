//
//  GravityWavesApp.h
//  GravityWaves
//
//  Created by Andrew Wright on 27/11/17.
//

#ifndef GravityWavesApp_h
#define GravityWavesApp_h

#include "cinder/app/App.h"

#include <Video/DepthCamera.h>
#include <Video/VideoPlayer.h>
#include <Video/SlopeFieldGenerator.h>
#include <Rendering/ParticleSystem.h>
#include <Rendering/Enities.h>
#include <Common/OscSender.h>

class GravityWavesApp : public ci::app::App
{
public:
    void                                setup            ( ) override;
    void                                update           ( ) override;
    void                                draw             ( ) override;
    
    void                                keyDown          ( ci::app::KeyEvent event ) override;
    
    void                                CreateEndgame    ( const E::Blackhole& master );
    float                               HeatMultiplierAt ( const ci::vec2& position ) const;
    void                                HandleOSCValues  ( const ci::Surface& slopeSurface );
    
    void                                Serialize        ( ci::JsonTree& tree );
    void                                Marshal          ( const ci::JsonTree& tree );
    
    std::vector<E::Blackhole>           _blackholes;
    std::vector<E::Attractor>           _attractors;
    
    ci::gl::GlslProgRef                 _blackHoleShader;
    ci::gl::TextureRef                  _backgroundImage;
    ci::gl::FboRef                      _galaxyBuffer;
    
    Video::VideoPlayerRef               _stars;
    Video::DepthCameraRef               _depthCamera;
    Video::SlopeFieldGeneratorRef       _slopeGenerator;
    ParticleSystemRef                   _particles;
    std::unique_ptr<E::Endgame>         _endgame;
    ci::gl::FboRef                      _rippleBuffer;
    ci::gl::GlslProgRef                 _rippleShader;
    ci::Channel8uRef                    _heatMask;
    OscSenderRef                        _oscSender;
    bool                                _renderUI{false};
};

#endif /* GravityWavesApp_h */
