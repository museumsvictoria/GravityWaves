#version 150

#define PI 3.14159265359

uniform ivec2 ciWindowSize;
uniform float ciElapsedSeconds;

uniform sampler2D uTexture;

struct BlackHole
{
    vec2 Position;
    float Strength;
    float Radius;
};

uniform BlackHole uBlackHoles[MAX_BLACK_HOLES];

uniform float uTimeOffset = 0.0;
uniform float uRotationPower = 0.0;
uniform float uPullConstant = 0.25;
uniform float uAspectRatio = 9.0 / 16.0;

in vec2 UV;

out vec4 FinalColor;

vec2 Rotate ( vec2 mt, vec2 st, float angle )
{
    float cos = cos ( ( angle + uTimeOffset ) * 1.0 );
    float sin = sin ( angle * uRotationPower );
    
    float nx = ( cos * ( st.x - mt.x ) ) + ( sin * ( st.y - mt.y ) ) + mt.x;
    float ny = ( cos * ( st.y - mt.y ) ) - ( sin * ( st.x - mt.x ) ) + mt.y;
    
    return vec2 ( nx, ny );
}

void main() 
{
    vec2 rAccum = vec2(0);
    float pullAccum = 0.0;

    for ( int i = 0; i < MAX_BLACK_HOLES; i++ )
    {
        if ( uBlackHoles[i].Radius == 0.0 ) continue;
        
        vec2 st = UV;
        vec2 mt = uBlackHoles[i].Position;

        float dx = st.x - mt.x;
        float dy = st.y - mt.y;

        dy *= uAspectRatio;

        float dist = sqrt ( dx * dx + dy * dy );
        float pull = ( uBlackHoles[i].Strength * uBlackHoles[i].Radius ) / ( dist * dist );

        pullAccum += pow(pull * 0.25, uPullConstant);

        vec2 r = Rotate ( mt, st, pull);
        rAccum += ( r - UV );
    }

    vec2 FinalUV = UV + rAccum;
    vec4 image = texture ( uTexture, FinalUV );
    image -= pullAccum;

    vec2 vel = (FinalUV - UV);

    FinalColor = vec4 ( image.rgb, 1.0 );
}