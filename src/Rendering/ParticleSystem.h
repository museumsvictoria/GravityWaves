//
//  ParticleSystem.h
//  GravityWaves
//
//  Created by Andrew Wright on 13/4/18.
//

#ifndef ParticleSystem_h
#define ParticleSystem_h

#include <Common/Types.h>

using ParticleSystemRef = std::unique_ptr<class ParticleSystem>;
class ParticleSystem
{
public:
    
    ParticleSystem      ( u32 maxParticles = 65535 );
    
    void                Inspect ( );
    
    void                Emit    ( u32 count, const ci::vec2& position, const ci::vec2& velocity );
    void                Update  ( float dt );
    void                Draw    ( );
    
    s32                 EmissionRate{0};
    float               SpawnThreshold{5.0f};
    float               NoiseMultiplier{0.1f};
    
protected:
    
    struct Particle
    {
        ci::vec2  P;
        ci::vec2  V;
        float     Age{0.0f};
        float     Life{0.0f};
        float     Size{1.0f};
        
        bool      Dead ( ) const;
    };
    
    std::vector<Particle>   _particles;
    ci::gl::VboMeshRef      _mesh;
    ci::gl::GlslProgRef     _shader;
    ci::gl::TextureRef      _texture;
    u32                     _liveParticles{0};
    u32                     _maxParticles{0};
    
    
};

#endif /* ParticleSystem_h */
