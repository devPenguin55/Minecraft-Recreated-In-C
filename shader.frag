#version 430 core

in vec2 fragUV;
flat in float fragLayer;
in vec3 worldPos;
flat in int fragGpuLightIndex;
flat in int fragFace;
out vec4 fragColor;

uniform sampler2DArray blockTextures;
uniform usamplerBuffer lightBuffer; // for all render chunks in one

#define FACE_TOP 1
#define FACE_BOTTOM 2
#define FACE_LEFT 3
#define FACE_RIGHT 4
#define FACE_FRONT 5
#define FACE_BACK 6
#define FACE_CROSS 7 // use for things like flowers
#define FACE_SLOPE 8 // use for slopes

float getFaceShade(int face)
{
    switch (face)
    {
        case FACE_TOP:
            return 1.0;
        case FACE_BOTTOM:
            return 0.9;
        case FACE_FRONT:
        case FACE_BACK:
            return 0.9;
        case FACE_LEFT:
        case FACE_RIGHT:
            return 0.9;
        case FACE_CROSS:
            return 1.0;  // billboards read as flat art, don't want them darkened
        default:
            return 0.9;
    }
}

float getLight(int x, int y, int z)
{
    int localX = ((x % 16) + 16) % 16;
    int localZ = ((z % 16) + 16) % 16;

    int localIndex =
        localX +
        localZ * 16 +
        y * 16 * 16;

    int finalIndex =
        fragGpuLightIndex * 32768 +
        localIndex;


    uint packedLightData = texelFetch(lightBuffer, finalIndex).r;

    uint skyLight = (packedLightData >> 4u) & 15u;
    uint blockLight = packedLightData & 15u;


    float sky = float(skyLight) / 15.0;
    float block = float(blockLight) / 15.0;
    float brightness = max(sky, block);
    return brightness;
}

void main()
{
    int layer = int(fragLayer);

    vec4 texColor; 

    // layer < 0 is for the UI to separate it from the normal rendering
    if (layer >= 0) {
        texColor = texture(blockTextures, vec3(fragUV, layer));
    } else {
        texColor = texture(blockTextures, vec3(fragUV, layer+55));
    }

    if (layer == 4)
    {
        texColor.a = 0.6;
    }

    if (texColor.a < 0.1)
    {
        discard;
    }

    if (layer < 0) {
        texColor.rgb *= 1.0;
    } else {        
        vec3 samplePos = worldPos;

        // first adjust the vertex that is on the half-voxel coordinates to the normal voxel coordinates
        switch (fragFace)
        {
            case FACE_TOP:
                samplePos.y -= 0.5;
                break;

            case FACE_BOTTOM:
                samplePos.y += 0.5;
                break;

            case FACE_LEFT:
                samplePos.x += 0.5;
                break;

            case FACE_RIGHT:
                samplePos.x -= 0.5;
                break;

            case FACE_FRONT:
                samplePos.z += 0.5;
                break;

            case FACE_BACK:
                samplePos.z -= 0.5;
                break;
        }

        float x = round(samplePos.x);
        float y = round(samplePos.y);
        float z = round(samplePos.z);

        // now sample for the neighboring block lighting to get the correct lighting for the face
        switch (fragFace)
        {
            case FACE_TOP:
                y += 1;
                break;

            case FACE_BOTTOM:
                y -= 1;
                break;

            case FACE_LEFT:
                x -= 1;
                break;

            case FACE_RIGHT:
                x += 1;
                break;

            case FACE_FRONT:
                z -= 1;
                break;

            case FACE_BACK:
                z += 1;
                break;
        }
        if (fragFace != FACE_CROSS && fragFace != FACE_SLOPE) {
            
            texColor.rgb *= getLight(int(floor(x)), int(floor(y)), int(floor(z))) * getFaceShade(fragFace);
        } else {
            texColor.rgb *= getLight(int(round(worldPos.x)), int(round(worldPos.y)), int(round(worldPos.z))) * getFaceShade(fragFace);
        }

        
    }

    

    fragColor = texColor;
    
}