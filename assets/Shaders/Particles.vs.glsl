#version 150
#define M_PI        3.14159265358979323846264338327950288   /* pi             */

uniform mat4 ciModelViewProjection;
uniform float uMaxSize = 64.0;

in vec4 ciPosition;
out float Age;

void main()
{
    Age = ciPosition.z;
    
    gl_Position = ciModelViewProjection * vec4 ( ciPosition.xy, 0, 1 );
    gl_PointSize = Age * ciPosition.w * uMaxSize;
        
    
}