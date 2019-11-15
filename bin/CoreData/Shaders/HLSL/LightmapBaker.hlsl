#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"
#include "ScreenPos.hlsl"
#include "Lighting.hlsl"
#include "Fog.hlsl"

void VS(float4 iPos : POSITION,
    #if !defined(BILLBOARD) && !defined(TRAILFACECAM)
        float3 iNormal : NORMAL,
    #endif
    float2 iTexCoord2 : TEXCOORD1,
    #ifdef SKINNED
        float4 iBlendWeights : BLENDWEIGHT,
        int4 iBlendIndices : BLENDINDICES,
    #endif
    #ifdef INSTANCED
        float4x3 iModelInstance : TEXCOORD4,
    #endif
    #if defined(BILLBOARD) || defined(DIRBILLBOARD)
        float2 iSize : TEXCOORD1,
    #endif

    out float3 oNormal : TEXCOORD1,
    out float4 oWorldPos : TEXCOORD2,
    out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    float2 lightmapUV = iTexCoord2 * cLMOffset.xy + cLMOffset.zw;

    oPos = float4(lightmapUV * float2(2, -2) + float2(-1, 1), 0, 1);
    oNormal = GetWorldNormal(modelMatrix);
    oWorldPos = float4(worldPos, 1.0);
}

void PS(
    float3 iNormal : TEXCOORD1,
    float4 iWorldPos : TEXCOORD2,

    out float4 oPosition : OUTCOLOR0,
    out float4 oSmoothPosition : OUTCOLOR1,
    out float4 oFaceNormal : OUTCOLOR2,
    out float4 oSmoothNormal : OUTCOLOR3)
{
    float3 normal = normalize(iNormal);

    oPosition = iWorldPos;
    oSmoothPosition = iWorldPos;
    oFaceNormal = float4(normal, 1.0);
    oSmoothNormal = float4(normal, 1.0);
}
