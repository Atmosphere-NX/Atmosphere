#version 460

layout (location = 0) in float inTileId;
layout (location = 1) in uvec2 inColorId;

layout (location = 0) out vec3 outTexCoord;
layout (location = 1) out vec4 outFrontPal;
layout (location = 2) out vec4 outBackPal;

layout (std140, binding = 0) uniform Config
{
    vec4 dimensions;
    vec4 vertices[3];
    vec4 palettes[24];
} u;

void main()
{
    float id = float(gl_InstanceID);
    float tileRow = floor(id / u.dimensions.z);
    float tileCol = id - tileRow * u.dimensions.z;

    vec2 basePos;
    basePos.x = 2.0 * (tileCol + 0.5) / u.dimensions.z - 1.0;
    basePos.y = 2.0 * (1.0 - (tileRow + 0.5) / u.dimensions.w) - 1.0;

    vec2 vtxData = u.vertices[gl_VertexID].xy;
    vec2 scale = vec2(1.0) / u.dimensions.zw;
    gl_Position.xy = vtxData * scale + basePos;
    gl_Position.zw = vec2(0.5, 1.0);

    outTexCoord = vec3(u.vertices[gl_VertexID].zw, inTileId);
    outFrontPal = u.palettes[inColorId.x];
    outBackPal  = u.palettes[inColorId.y];
}
