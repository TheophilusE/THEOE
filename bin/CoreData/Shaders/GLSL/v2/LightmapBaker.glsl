#define NUM_RENDER_TARGETS 6
#define URHO3D_PIXEL_NEED_NORMAL
#define CUSTOM_MATERIAL_CBUFFER

#include "_Config.glsl"
#include "_GammaCorrection.glsl"

UNIFORM_BUFFER_BEGIN(4, Material)
    UNIFORM(float cLightmapLayer)
    UNIFORM(float cLightmapGeometry)
    UNIFORM(vec2 cLightmapPositionBias)

    UNIFORM(vec4 cUOffset)
    UNIFORM(vec4 cVOffset)
    UNIFORM(vec4 cLMOffset)

    UNIFORM(vec4 cMatDiffColor)
    UNIFORM(vec3 cMatEmissiveColor)
UNIFORM_BUFFER_END()

#include "_Uniforms.glsl"
#include "_Samplers.glsl"
#include "_VertexLayout.glsl"
#include "_PixelOutput.glsl"

#include "_VertexTransform.glsl"

VERTEX_OUTPUT(vec2 vTexCoord)
VERTEX_OUTPUT(vec3 vNormal)
VERTEX_OUTPUT(vec3 vWorldPos)
VERTEX_OUTPUT(vec4 vMetadata)

#ifdef URHO3D_VERTEX_SHADER
void main()
{
    VertexTransform vertexTransform = GetVertexTransform();
    vec2 lightmapUV = iTexCoord1 * cLMOffset.xy + cLMOffset.zw;

#ifdef URHO3D_FLIP_FRAMEBUFFER
    const vec4 scaleOffset = vec4(2, -2, -1, 1);
#else
    const vec4 scaleOffset = vec4(2, 2, -1, -1);
#endif

    gl_Position = vec4(lightmapUV * scaleOffset.xy + scaleOffset.zw, cLightmapLayer, 1);
    vNormal = vertexTransform.normal;
    vWorldPos = vertexTransform.position.xyz;
    vMetadata = vec4(cLightmapGeometry, cLightmapPositionBias.x, cLightmapPositionBias.y, 0.0);
    vTexCoord = GetTransformedTexCoord();
}
#endif

#ifdef URHO3D_PIXEL_SHADER
void main()
{
#ifdef URHO3D_MATERIAL_HAS_DIFFUSE
    vec4 albedoInput = texture2D(sDiffMap, vTexCoord);
    vec4 albedo = DiffMap_ToLinear(cMatDiffColor * albedoInput);
#else
    vec4 albedo = GammaToLinearSpaceAlpha(cMatDiffColor);
#endif

    vec3 emissiveColor = cMatEmissiveColor;
    #ifdef EMISSIVEMAP
        emissiveColor *= texture2D(sEmissiveMap, vTexCoord.xy).rgb;
    #endif

    vec3 normal = normalize(vNormal);

    vec3 dPdx = dFdx(vWorldPos.xyz);
    vec3 dPdy = dFdy(vWorldPos.xyz);
    vec3 faceNormal = normalize(cross(dPdx, dPdy));
    if (dot(faceNormal, normal) < 0)
        faceNormal *= -1.0;

    vec3 dPmax = max(abs(dPdx), abs(dPdy));
    float texelRadius = max(dPmax.x, max(dPmax.y, dPmax.z)) * (1.4142135 * 0.5);

    float scaledBias = vMetadata.y;
    float constBias = vMetadata.z;
    vec3 biasScale = max(abs(vWorldPos.xyz), vec3(1.0, 1.0, 1.0));
    vec3 position = vWorldPos.xyz + sign(faceNormal) * biasScale * scaledBias + faceNormal * constBias;

    gl_FragData[0] = vec4(position, vMetadata.x);
    gl_FragData[1] = vec4(position, texelRadius);
    gl_FragData[2] = vec4(faceNormal, 1.0);
    gl_FragData[3] = vec4(normal, 1.0);
    gl_FragData[4] = vec4(albedo.rgb * albedo.a, 1.0);
    gl_FragData[5] = vec4(emissiveColor.rgb, 1.0);
}
#endif
