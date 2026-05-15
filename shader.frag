#version 330 core
in vec2 fragUV;
flat in float fragLayer;
in float fragBrightness;

out vec4 fragColor;

uniform sampler2DArray blockTextures;

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

    texColor.rgb *= fragBrightness;
    fragColor = texColor;
}