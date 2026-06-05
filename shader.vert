#version 330 compatibility

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in float layer;
layout(location = 3) in int gpuLightIndex;
layout(location = 4) in int face;


out vec2 fragUV;
flat out float fragLayer;
out vec3 worldPos;
flat out int fragGpuLightIndex;
flat out int fragFace;

void main()
{
    fragUV = texCoord;
    fragLayer = layer;
    worldPos = position;
    fragGpuLightIndex = gpuLightIndex;
    fragFace = face;
    
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
}