#version 150

uniform sampler2D 	uTexture;
uniform float 	  	uTime;
uniform float 	  	uAmplitude;
uniform float 		uSpeed;
uniform vec2        uEpicenter;
uniform float       uAlpha;
uniform float       uRadiusOfEffect;

in vec2 			UV;

out vec4 FinalColor;

void main()
{
	vec2 c = uEpicenter;
	c.x = clamp(c.x, 0.0, 1.0);
	c.y = clamp(c.y, 0.0, 1.0);

	vec2 p = UV;
	vec2 dir = p - c;
	float dist = length(dir);

    vec2 offset = vec2(0);
    if ( dist < uRadiusOfEffect )
    {
	    offset = dir * ( sin ( uTime * dist * uAmplitude - uTime * uSpeed)) / 5.0;
    }

	vec4 from = texture ( uTexture, p + offset );
	FinalColor = vec4 ( from.rgb, from.a * uAlpha );
}