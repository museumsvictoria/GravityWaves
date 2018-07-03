#version 150

uniform sampler2D uTexture;

in float Age;

out vec4 FinalColor;

void main ( )
{
    FinalColor = texture ( uTexture, gl_PointCoord.xy );
    FinalColor.a *= 1.0f - Age;
}
