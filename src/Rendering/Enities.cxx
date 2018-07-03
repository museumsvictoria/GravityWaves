//
//  Enities.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 13/4/18.
//

#include <Rendering/Enities.h>
#include <Common/Constants.h>
#include "cinder/Rand.h"

using namespace ci;

namespace E
{
    ///
    /// Utils
    ///
    
    vec2 RandomPositionInRadius ( float minRadius )
    {
        float angle = randFloat ( M_PI * 2.0f );
        float radius = randFloat ( minRadius, minRadius * 1.3f );
        
        vec2 v = app::getWindowCenter();
        v.x += std::cos ( angle ) * radius;
        v.y += std::sin ( angle ) * radius;
        
        return v;
    }
    
    ///
    /// DynamicMass
    ///

    void DynamicMass::Interact ( const DynamicMass& other, float dt )
    {
        if ( IsInEndGame || other.IsInEndGame ) return;
        
        vec2 delta = other.Position - Position;
        float d = glm::distance(other.Position, Position);
        float d2 = d*d;
        float radius = std::max ( other.Radius, Radius );
        
        if ( d > radius )
        {
            Velocity.x += K::G * other.GetMass() / d2 * ( delta.x ) / d;
            Velocity.y += K::G * other.GetMass() / d2 * ( delta.y ) / d;
        }
    }
    
    ///
    /// Blackhole
    ///
    
    Blackhole::Blackhole ( )
    {
        Respawn ( );
    }
    
    void Blackhole::Tick ( float dt )
    {
        const float kPadding = 150;
        
        History.push_back ( Position );
        Position += Velocity * dt;
        
        if ( !IsInEndGame )
        {
            if ( Position.x < -kPadding - Radius ) Respawn();
            if ( Position.x > app::getWindowWidth() + kPadding + Radius  ) Respawn();
            
            if ( Position.y < -kPadding - Radius ) Respawn();
            if ( Position.y > app::getWindowHeight() + kPadding + Radius ) Respawn();
        }
    }
    
    void Blackhole::Respawn ( )
    {
        Heat = 0.0f;
        History.clear();
        
        IsInEndGame = false;
        
        Position = RandomPositionInRadius ( app::getWindowSize().y * 0.48f );
        Velocity = glm::normalize( vec2 ( app::getWindowSize() ) - Position ) * randFloat ( 0.9f, 1.1f );
        
        Mass = randFloat ( K::MinBlackholeMass, K::MaxBlackholeMass );
        Radius = 0.0f;
        
        RadiusRatio = lmap ( Mass, K::MinBlackholeMass, K::MaxBlackholeMass, 0.0f, 1.0f);
        Strength = lerp(0.01f, 0.01f, RadiusRatio);
        
        float time = lerp(1.0f, 5.0f, RadiusRatio);
        float radius = lerp ( K::MinBlackholeRadius, K::MaxBlackholeRadius, RadiusRatio );
        
        app::timeline().apply(&Radius, radius, time, EaseInOutQuint());
    }
    
    void Blackhole::Draw ( )
    {
        gl::VertBatch batch(GL_LINE_STRIP);
        for (auto& p : History) batch.vertex(p);
        batch.draw();
        
        gl::ScopedColor c { lerp ( Colorf::white(), Colorf ( 1, 0, 0 ), Heat ) };
        gl::drawSolidCircle(Position, Radius);
    }
    
    void Blackhole::DebugDraw ( )
    {
        if ( K::AllowCollisions )
        {
            gl::ScopedColor c { Colorf ( 0, 0, 1 ) };
            gl::drawSolidCircle( Position, Radius * K::CollisionAllowance );
        }
        
        static gl::TextureFontRef kFont = gl::TextureFont::create( Font ( "Helvetica", 12 ) );;
        gl::ScopedColor c { Colorf::black() };
        
        auto heatStr = std::to_string ( Heat );
        auto size = kFont->measureString ( heatStr );
        kFont->drawString( heatStr, vec2 ( Position - vec2 ( size.x / 2.0f, 0 ) ) );
    }
    
    ///
    /// Endgame
    ///
    
    Endgame::Endgame ( const std::vector<Blackhole *>& blackholes )
    : Blackholes ( blackholes )
    {
        Velocity.x = randFloat(-1, 1);
        Velocity.y = randFloat(-1, 1);
        
        _life = randFloat(5.0f, 8.0f);
    }
    
    void Endgame::Explode ( )
    {
        for ( auto& b : Blackholes )
        {
            b->Respawn();
        }
    }
    
    
    void Endgame::Tick ( float dt )
    {
        _age += dt;
        Position += Velocity;
        
        float life = _age / _life;
        
        for ( auto& b : Blackholes )
        {
            float angle = b->Angle * SpinDirection;
            float r = lerp ( b->BaseDistance * 1.80f, 0.0f, life );
            
            vec2 v;
            v.x = Position.x + std::cos ( angle ) * r;
            v.y = Position.y + std::sin ( angle ) * r;
            
            b->Position = lerp ( b->Position, v, 0.08f );
            b->Radius = lerp ( b->BaseRadius, 0.0f, life );
            b->Radius.stop();
            b->Angle += lerp ( 0.06f, 0.5f, easeInQuint(life) );
        }
    }
    
}
