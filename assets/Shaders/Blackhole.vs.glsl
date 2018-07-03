#version 150

uniform mat4 ciModelViewProjection;
uniform ivec2 ciWindowSize;

in vec4 ciPosition;
in vec2 ciTexCoord0;

out vec2 UV;
out vec2 UVNorm;

void main ( )
{
    gl_Position = ciModelViewProjection * ciPosition;
    UV = ciTexCoord0;
    UVNorm = UV / vec2(ciWindowSize);
}