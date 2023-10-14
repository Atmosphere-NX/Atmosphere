#version 460

layout (location = 0) noperspective in vec3 inTexCoord;
layout (location = 1) flat in vec4 inFrontPal;
layout (location = 2) flat in vec4 inBackPal;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform sampler2DArray tileset;

void main()
{
    float value = texture(tileset, inTexCoord).r;
    outColor = mix(inBackPal, inFrontPal, value);
}
