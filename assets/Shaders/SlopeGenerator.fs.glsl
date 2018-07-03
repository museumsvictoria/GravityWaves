#version 150

uniform sampler2D uDepthBuffer;
uniform float uMaxDepth = 1.0;
uniform float uMinDepth = 1.0;
uniform float uPeakThreshold = 1.0f;

in vec2 UV;
out vec4 SlopeColor;

#define OP max

#define M_PI        3.14159265358979323846264338327950288

float GetDepth ( vec2 uv )
{  
    return texture ( uDepthBuffer, uv ).r;
}

const vec2 texel = vec2 ( 1.0 / 512.0, 1.0 / 424.0 );
const vec2 kernel[] = vec2[] ( vec2 ( 0.0, texel.y ), vec2 ( 0.0, -texel.y ), vec2 ( texel.x, 0.0 ), vec2 ( -texel.x, 0.0 ), vec2 ( -texel.x, -texel.y ), vec2 ( texel.x, -texel.y ), vec2 ( -texel.x, texel.y ), vec2 ( texel.x, texel.y ) );

float lmap(float val, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + ((outMax - outMin) * (val - inMin)) / (inMax - inMin);
}

vec2 lmap(vec2 val, vec2 inMin, vec2 inMax, vec2 outMin, vec2 outMax)
{
    return outMin + ((outMax - outMin) * (val - inMin)) / (inMax - inMin);
}

void main()
{   
    vec2 uv = UV; 
    SlopeColor.a = 1.0;
    
    float MyDepth = GetDepth( uv );
    float peak = MyDepth < uPeakThreshold ? 1.0 : 0.0;

    if ( MyDepth > uMinDepth )
    {
        SlopeColor.rgb = vec3(0.5, 0.5, 0);
        return;
    }

    int index = -1;
    float closest = 0.0f;

    for ( int i = 0; i < 8; i++ )
    {
        float d = GetDepth ( uv + kernel[i] );
        if ( d > closest )
        {
            index = i;
            closest = d;
        }
    }
    
    if ( MyDepth > closest )
    {
        SlopeColor.rgb = vec3(0.5, 0.5, 0.0);
    }else
    {    
        vec2 R = closest > -1 ? normalize ( kernel[index] ) : vec2(0);
        if ( closest > -1 )
        {
            R.y *= -1.0f;
        }

        SlopeColor.rgb = vec3( lmap(R, vec2(-1), vec2(1), vec2(1), vec2(0)), peak ); // * 0.5 + 0.5;
        SlopeColor.a = 1.0;
    }
}