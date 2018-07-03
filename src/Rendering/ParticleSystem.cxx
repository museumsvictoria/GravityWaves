//
//  ParticleSystem.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 13/4/18.
//

#include <Rendering/ParticleSystem.h>
#include "cinder/Rand.h"
#include "cinder/Perlin.h"
#include "CinderImGui.h"

using namespace ci;

static Perlin kNoise;

bool ParticleSystem::Particle::Dead ( ) const
{
    return P.x < 0 || P.x > app::getWindowWidth() || P.y < 0 || P.y > app::getWindowHeight() || Age >= Life;
}

ParticleSystem::ParticleSystem ( u32 maxParticles )
: _maxParticles ( maxParticles )
{
    _particles.reserve ( maxParticles );
    
    gl::VboMesh::Layout layout;
    layout.attrib( geom::POSITION, 4 );
    
    _mesh = gl::VboMesh::create ( maxParticles, GL_POINTS, { layout } );
    _texture = gl::Texture::create ( loadImage( app::loadAsset( "Textures/Particle.png" ) ), gl::Texture::Format().mipmap().minFilter(GL_LINEAR_MIPMAP_LINEAR).magFilter(GL_LINEAR));
    _shader = gl::GlslProg::create( app::loadAsset( "Shaders/Particles.vs.glsl" ), app::loadAsset( "Shaders/Particles.fs.glsl" ) );
}

void ParticleSystem::Inspect ( )
{
    if ( ui::CollapsingHeader ( "Particle System" ) )
    {
        ui::Text ( "%d Live Particles | %d Max Particles", _liveParticles, _maxParticles );
        ui::DragInt ( "Emission Rate", &EmissionRate, 0, 0, 32 );
        ui::DragFloat ( "Min. Spawn Velocity", &SpawnThreshold, 0.01f, 0.0f, 32.0f );
        ui::DragFloat ( "Noise Multiplier", &NoiseMultiplier, 0.001f, 0.0f, 3.0f );
    }
}

void ParticleSystem::Emit ( u32 count, const ci::vec2& position, const ci::vec2& velocity )
{
    for ( int i = 0; i < count; i++ )
    {
        if ( _particles.size() < _maxParticles )
        {
            _particles.push_back ( {} );
            
            auto& p = _particles.back();
            p.P = position;
            p.V = vec2 ( randFloat(-1, 1), randFloat(-1, 1) ) + velocity;
            p.Life = randFloat ( 1.0f, 3.0f );
            p.Size = randFloat ( 1.0f );
        }
    }
}

void ParticleSystem::Update ( float dt )
{
    _liveParticles = 0;

    auto p = _mesh->mapAttrib4f( geom::POSITION );
    auto it = _particles.begin();
    while ( it != _particles.end() )
    {
        if ( it->Dead() )
        {
            it = _particles.erase ( it );
        }else
        {
            it->P += it->V;
            it->Age += dt;
            it->V += kNoise.dfBm ( it->P * 0.02f ) * NoiseMultiplier;
            
            *p++ = vec4 ( it->P, it->Age / it->Life, it->Size );
            _liveParticles++;
            
            it++;
        }
    }
    
    p.unmap();
}

void ParticleSystem::Draw ( )
{
    if ( _liveParticles > 0 )
    {
        gl::ScopedColor color { Colorf::white() };
        gl::ScopedState state { GL_VERTEX_PROGRAM_POINT_SIZE, true };
        gl::ScopedGlslProg shader { _shader };
        gl::ScopedTextureBind tex0 { _texture, 0 };
        gl::ScopedBlendAdditive blend;
        gl::draw ( _mesh, 0, _liveParticles );
    }
}
