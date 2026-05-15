#version 330 compatibility

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in float layer;
layout(location = 3) in float brightness;

out vec2 fragUV;
flat out float fragLayer;
out float fragBrightness;

void main()
{
    fragUV = texCoord;
    fragLayer = layer;
    fragBrightness = brightness;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
}