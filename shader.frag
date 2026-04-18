#version 330 core
in vec2 fragUV;
flat in int fragLayer;

out vec4 fragColor;

uniform sampler2DArray blockTextures;

void main()
{
    vec4 texColor = texture(blockTextures, vec3(fragUV, fragLayer));

    if (fragLayer == 4)
    {
        texColor.a = 0.6;
    }

    if (texColor.a < 0.1)
    {
        discard;
    }

    fragColor = texColor;
}