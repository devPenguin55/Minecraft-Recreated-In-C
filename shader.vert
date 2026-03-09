#version 330 compatibility

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in int layer;

out vec2 fragUV;
flat out int fragLayer;

void main()
{
    fragUV = texCoord;
    fragLayer = layer;

    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
}