#version 330 core
in vec2 fragUV;
flat in int fragLayer;

out vec4 fragColor;

uniform sampler2DArray blockTextures;

void main() {
    fragColor = texture(blockTextures, vec3(fragUV, float(fragLayer)));
}