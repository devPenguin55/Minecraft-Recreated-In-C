#version 430 core

in vec2 fragUV;
flat in float fragLayer;
in vec3 worldPos;

out vec4 fragColor;

uniform sampler2DArray blockTextures;
uniform usamplerBuffer lightBuffer;

void main()
{
    int layer = int(fragLayer);
    vec4 texColor = texture(blockTextures, vec3(fragUV, layer));

    if (layer == 4)
    {
        texColor.a = 0.6;
    }

    if (texColor.a < 0.1)
    {
        discard;
    }

    
    // example lookup
    int x = int(round(worldPos.x));
    int y = int(round(worldPos.y));
    int z = int(round(worldPos.z));

    int chunkX = int(round(float(x) / 16.0));
    int chunkZ = int(round(float(z) / 16.0));

    int localX = ((x % 16) + 16) % 16;
    int localZ = ((z % 16) + 16) % 16;

    int localIndex =
        localX +
        localZ * 16 +
        y * 16 * 16;

    // TEMP:
    // replace this with actual chunk->gpuLightIndex lookup later
    int gpuLightIndex = 96;

    int finalIndex =
        gpuLightIndex * 32768 +
        localIndex;


    uint packedLightData = texelFetch(lightBuffer, finalIndex).r;

    uint skyLight = (packedLightData >> 4u) & 15u;
    uint blockLight = packedLightData & 15u;


    float sky = float(skyLight) / 15.0;
    float block = float(skyLight) / 15.0;
    float brightness = sky;


    texColor.rgb *= brightness;

    


    fragColor = texColor;
    
}