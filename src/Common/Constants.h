//
//  Constants.h
//  GravityWaves
//
//  Created by Andrew Wright on 13/4/18.
//

// Before you judge me for the constant soup that follows - i had to develop this
// software in sydney while it was being tested in melbourne on a very non standard
// installation surface. Every property possible needed to be real-time tweakable
// in order to not completely tank our iteration time. I know it's bad, but it was
// necessary. ;)
// -A

#ifndef Constants_h
#define Constants_h

#include "cinder/Timeline.h"
#include "cinder/Json.h"

struct K
{
    static bool             DebugDrawMasses;
    static bool             AllowCollisions;
    static bool             HardCollisions;
    static bool             ReactToVectorField;
    static bool             DrawHeatMask;
    static bool             DrawField;
    static bool             DrawSlopeMap;
    static bool             DrawPointAttractors;
    static bool             AnnabelBeingAPest;
    static bool             RenderUI;
    static int              EmissionRateDuringEndgame;
    
    static float            CollisionAllowance;
    static float            HeatRate;
    static float            CoolRate;
    static float            VelocityMultiplier;
    static float            EndgameCaptureRadius;
    static ci::Anim<float>  RippleAlpha;
    static ci::Anim<float>  RippleAmplitude;
    static ci::Anim<float>  RippleSpeed;
    
    static float            AttractorSampleSpacing;
    static float            AttractorSampleMass;
    
    static float            G;
    static float            MinBlackholeRadius;
    static float            MaxBlackholeRadius;
    static float            MinBlackholeMass;
    static float            MaxBlackholeMass;
    
    static float            RotationPower;
    static float            PullConstant;
    static int              MaxBlackHoles;
    
    static std::string      OSCEndpoint;
    static int              OSCPort;
    static int              ScreenID;
    static float            UserPressednessThreshold;
    static std::string      OSCPrefix;
    static int              OSCSendDelay;
    static bool             OSCSendAllPacked;
    
    static void             Inspect ( );
    static void             Save    ( ci::JsonTree& tree );
    static void             Load    ( const ci::JsonTree& tree );
};

#endif /* Constants_h */
