//
//  Constants.cxx
//  GravityWaves
//
//  Created by Andrew Wright on 13/4/18.
//

#include <Common/Constants.h>
#include "CinderImGui.h"

using namespace ci;

bool            K::DebugDrawMasses           = false;
bool            K::AllowCollisions           = true;
bool            K::HardCollisions            = true;
bool            K::ReactToVectorField        = true;
bool            K::DrawHeatMask              = false;
bool            K::DrawField                 = false;
bool            K::DrawSlopeMap              = false;
bool            K::DrawPointAttractors       = false;
bool            K::AnnabelBeingAPest         = true;
bool            K::RenderUI                  = false;
int             K::EmissionRateDuringEndgame = 3;

float           K::CollisionAllowance        = 0.85f;
float           K::HeatRate                  = 1.1f;
float           K::CoolRate                  = 0.99f;
float           K::VelocityMultiplier        = 0.4f;
float           K::EndgameCaptureRadius      = 200.0f;
Anim <float>    K::RippleAlpha               = 1.0f;
Anim <float>    K::RippleAmplitude           = 50.0f;
Anim <float>    K::RippleSpeed               = 13.0f;

float           K::AttractorSampleSpacing    = 30.0f;
float           K::AttractorSampleMass       = 70000.0f;

float           K::G = 0.03;
float           K::MinBlackholeRadius        = 30.0f;
float           K::MaxBlackholeRadius        = 90.0f;
float           K::MinBlackholeMass          = 20000.0f;
float           K::MaxBlackholeMass          = 25000.0f;

float           K::RotationPower             = 0.0f;
float           K::PullConstant              = 1.40f;
int             K::MaxBlackHoles             = 3;

std::string     K::OSCEndpoint              = "127.0.0.1";
int             K::OSCPort                  = 9898;
int             K::ScreenID                 = 0;
float           K::UserPressednessThreshold = 0.3f;
std::string     K::OSCPrefix                = "/bp/cosmic/screen_id/";
int             K::OSCSendDelay             = 1;
bool            K::OSCSendAllPacked         = true;

void K::Inspect ( )
{
    ui::Checkbox ( "Annabel being a pest?", &AnnabelBeingAPest );
    ui::Checkbox ( "Draw Slope Field", &DrawField ); ui::SameLine();
    ui::Checkbox ( "Draw Slope Map", &DrawSlopeMap ); ui::SameLine();
    ui::Checkbox ( "Draw Masses", &DebugDrawMasses ); ui::SameLine();
    ui::Checkbox ( "Draw Heat Mask", &DrawHeatMask ); ui::SameLine();
    ui::Checkbox ( "Draw Point Attractors", &DrawPointAttractors );
    ui::Checkbox ( "React To Slope Field", &ReactToVectorField );
    ui::Checkbox ( "Allow Physical Collisions", &AllowCollisions );
    
    if ( AllowCollisions )
    {
        ui::Indent(10);
        ui::Checkbox( "Rigid Collision Response", &HardCollisions );
        ui::DragFloat( "Radial Overlap Allowance", &CollisionAllowance, 0.001f, 0.0f, 1.0f );
        ui::DragFloat( "Heat Rate", &HeatRate, 0.001f, 1.0f, 2.0f );
        ui::DragFloat( "Cool Rate", &CoolRate, 0.001f, 0.0f, 1.0f );
        ui::Unindent(10);
    }
    
    if ( ui::CollapsingHeader ( "Physics World" ) )
    {
        ui::Indent(10);
        ui::DragFloat( "Min Blackhole Mass", &MinBlackholeMass, 1.0f, 0.0f, MaxBlackholeRadius );
        ui::DragFloat( "Max Blackhole Mass", &MaxBlackholeMass, 1.0f, MinBlackholeMass, 100000.0f );
        ui::DragFloat( "Max Blackhole Radius", &MinBlackholeRadius, 0.1, 0.0f, MaxBlackholeRadius );
        ui::DragFloat( "Min Blackhole Radius", &MaxBlackholeRadius, 0.1f, MinBlackholeMass, 256.0f );
        ui::DragFloat( "Rotation Power", &RotationPower, 0.01f, 0.0f, 16.0f );
        ui::Unindent(10);
    }
    
    if ( ui::CollapsingHeader ( "Gravity Waves" ) )
    {
        ui::Indent(10);
        ui::DragFloat( "Ripple Amplitude", &RippleAmplitude(), 0.01f, 0.0f, 512.0f );
        ui::DragFloat( "Ripple Speed", &RippleSpeed(), 0.01f, 0.0f, 16.0f );
        ui::Unindent(10);
    }
    
    ui::DragFloat ( "G", &G, 0.0001f, 0.0f, 0.5f );
    ui::DragFloat ( "Velocity Mult.", &VelocityMultiplier, 0.0001f, -1.0f, 1.0f );
    ui::DragFloat ( "Peak Attractor Sample Spacing", &AttractorSampleSpacing, 1.0f, 8.0f, 256.0f );
    ui::DragFloat ( "Peak Attractor Mass", &AttractorSampleMass, 10.0f, 1000.0f, 100000.0f );
}

template <typename T>
inline void Write_ ( JsonTree& tree, const std::string& name, const T& p )
{
    tree.pushBack ( JsonTree ( name, p ) );
}
    
void K::Save ( JsonTree& tree )
{
    #define W(n, t) Write_(tree, n, t)
    
    W("AnnabelPest", AnnabelBeingAPest);
    W("DrawSlopeField", DrawField);
    W("DrawSlopeMap", DrawSlopeMap);
    W("DrawHeatMask", DrawHeatMask);
    W("DrawPointAttractors", DrawPointAttractors);
    W("ReactToSlopeField", ReactToVectorField);
    W("AllowPhysicalCollisions", AllowCollisions);
    W("RigidCollisionResponse", HardCollisions);
    W("RadialOverlapAllowance", CollisionAllowance);
    W("HeatRate", HeatRate);
    W("CoolRate", CoolRate);
    W("MinBlackholeMass", MinBlackholeMass);
    W("MaxBlackholeMass", MaxBlackholeMass);
    W("MinBlackholeRadius", MinBlackholeRadius);
    W("MaxBlackholeRadius", MaxBlackholeRadius);
    W("RotationPower", RotationPower);
    W("MaxBlackHoles", MaxBlackHoles);
    W("PullConstant", PullConstant);
    W("SlopeGenVelocityMultiplier", VelocityMultiplier);
    W("PeakAttractorSampleSpacing", AttractorSampleSpacing);
    W("PeakAttractorSampleMass", AttractorSampleMass);
    W("EndgameParticleEmissionRate", EmissionRateDuringEndgame);
    W("OSCEndpoint", OSCEndpoint );
    W("OSCPort", OSCPort );
    W("OSCSendFrameDelay", OSCSendDelay);
    W("OSCSendAllPacked", OSCSendAllPacked);
    W("ScreenID", ScreenID );
    W("PressednessThreshold", UserPressednessThreshold );
    
#undef W
}
    
void K::Load ( const JsonTree& tree )
{
    #define R(n, v) if(tree.hasChild(n)) v = tree[n].getValue<decltype(v)>()
    
    R("AnnabelPest", AnnabelBeingAPest);
    R("DrawSlopeField", DrawField);
    R("DrawSlopeMap", DrawSlopeMap);
    R("DrawHeatMask", DrawHeatMask);
    R("DrawPointAttractors", DrawPointAttractors);
    R("ReactToSlopeField", ReactToVectorField);
    R("AllowPhysicalCollisions", AllowCollisions);
    R("RigidCollisionResponse", HardCollisions);
    R("RadialOverlapAllowance", CollisionAllowance);
    R("HeatRate", HeatRate);
    R("CoolRate", CoolRate);
    R("MinBlackholeMass", MinBlackholeMass);
    R("MaxBlackholeMass", MaxBlackholeMass);
    R("MinBlackholeRadius", MinBlackholeRadius);
    R("MaxBlackholeRadius", MaxBlackholeRadius);
    R("RotationPower", RotationPower);
    R("MaxBlackHoles", MaxBlackHoles);
    R("PullConstant", PullConstant);
    R("SlopeGenVelocityMultiplier", VelocityMultiplier);
    R("PeakAttractorSampleSpacing", AttractorSampleSpacing);
    R("PeakAttractorSampleMass", AttractorSampleMass);
    R("EndgameParticleEmissionRate", EmissionRateDuringEndgame);
    
    R("OSCEndpoint", OSCEndpoint );
    R("OSCPort", OSCPort );
    R("OSCSendFrameDelay", OSCSendDelay);
    R("OSCSendAllPacked", OSCSendAllPacked);
    R("ScreenID", ScreenID );
    R("PressednessThreshold", UserPressednessThreshold );
    
    OSCPrefix = "/bp/cosmic/" + std::to_string(ScreenID) + "/";
    
    #undef R
}

