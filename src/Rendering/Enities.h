//
//  Enities.h
//  GravityWaves
//
//  Created by Andrew Wright on 13/4/18.
//

#ifndef Enities_h
#define Enities_h

#include <boost/circular_buffer.hpp>
#include "cinder/Timeline.h"

namespace E
{
    using PositionHistory = boost::circular_buffer<ci::vec2>;
    struct DynamicMass
    {
        inline float                GetMass     ( ) const { return Mass; }
        void                        Interact    ( const DynamicMass& other, float dt );
        
        ci::vec2                    Position;
        ci::vec2                    Velocity;
        float                       Mass{1.0f};
        ci::Anim<float>             Radius{1.0f};
        float                       BaseRadius{1.0f};
        float                       BaseDistance{1.0f};
        float                       RadiusRatio{1.0f};
        float                       Strength{0.01f};
        float                       Heat{0.01f};
        bool                        IsInEndGame{false};
    };
    
    struct Blackhole                : public DynamicMass
    {
        Blackhole                   ( );
        
        void                        Tick        ( float dt);
        void                        Draw        ( );
        void                        DebugDraw   ( );
        void                        Respawn     ( );
        
        float                       Age{0.0f};
        float                       Angle{0.0f};
        PositionHistory             History{100};
    };
    
    struct Attractor                : public DynamicMass { };
    
    struct Endgame                  : public DynamicMass
    {
        Endgame                     ( const std::vector<Blackhole *>& blackholes );
        
        inline float                Life        ( ) const { return _age / _life; }
        inline float                TotalLife   ( ) const { return _life; }
        inline bool                 Dead        ( ) const { return _age >= _life; };
        
        void                        Explode     ( );
        void                        Tick        ( float dt );
                
        std::vector<Blackhole *>    Blackholes;
        float                       SpinDirection{1.0f};
        
    protected:
        
        float                       _age{0.0f};
        float                       _life{0.0f};
    };
}

#endif /* Enities_h */
