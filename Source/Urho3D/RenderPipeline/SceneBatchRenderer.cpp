//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Zone.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/LightProcessor.h"
#include "../RenderPipeline/SceneBatchRenderer.h"
#include "../Scene/Scene.h"

#include <EASTL/sort.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

/// Return shader parameter for camera depth mode.
Vector4 GetCameraDepthModeParameter(const Camera* camera)
{
    Vector4 depthMode = Vector4::ZERO;
    if (camera->IsOrthographic())
    {
        depthMode.x_ = 1.0f;
#ifdef URHO3D_OPENGL
        depthMode.z_ = 0.5f;
        depthMode.w_ = 0.5f;
#else
        depthMode.z_ = 1.0f;
#endif
    }
    else
        depthMode.w_ = 1.0f / camera->GetFarClip();
    return depthMode;
}

/// Return shader parameter for camera depth reconstruction.
Vector4 GetCameraDepthReconstructParameter(const Camera* camera)
{
    const float nearClip = camera->GetNearClip();
    const float farClip = camera->GetFarClip();
    return {
        farClip / (farClip - nearClip),
        -nearClip / (farClip - nearClip),
        camera->IsOrthographic() ? 1.0f : 0.0f,
        camera->IsOrthographic() ? 0.0f : 1.0f
    };
}

/// Return shader parameter for zone fog.
Vector4 GetFogParameter(const Camera* camera)
{
    const float farClip = camera->GetFarClip();
    float fogStart = Min(camera->GetEffectiveFogStart(), farClip);
    float fogEnd = Min(camera->GetEffectiveFogStart(), farClip);
    if (fogStart >= fogEnd * (1.0f - M_LARGE_EPSILON))
        fogStart = fogEnd * (1.0f - M_LARGE_EPSILON);
    const float fogRange = Max(fogEnd - fogStart, M_EPSILON);
    return {
        fogEnd / farClip,
        farClip / fogRange,
        0.0f,
        0.0f
    };
}

/// Add frame-specific shader parameters.
void AddFrameShaderParameters(DrawCommandQueue& drawQueue, const FrameInfo& frameInfo, const Scene* scene)
{
    drawQueue.AddShaderParameter(VSP_DELTATIME, frameInfo.timeStep_);
    drawQueue.AddShaderParameter(PSP_DELTATIME, frameInfo.timeStep_);

    const float elapsedTime = scene->GetElapsedTime();
    drawQueue.AddShaderParameter(VSP_ELAPSEDTIME, elapsedTime);
    drawQueue.AddShaderParameter(PSP_ELAPSEDTIME, elapsedTime);
}

/// Add camera-specific shader parameters.
void AddCameraShaderParameters(DrawCommandQueue& drawQueue, const Camera* camera, float constantDepthBias)
{
    const Matrix3x4 cameraEffectiveTransform = camera->GetEffectiveWorldTransform();
    drawQueue.AddShaderParameter(VSP_CAMERAPOS, cameraEffectiveTransform.Translation());
    drawQueue.AddShaderParameter(VSP_VIEWINV, cameraEffectiveTransform);
    drawQueue.AddShaderParameter(VSP_VIEW, camera->GetView());
    drawQueue.AddShaderParameter(PSP_CAMERAPOS, cameraEffectiveTransform.Translation());

    const float nearClip = camera->GetNearClip();
    const float farClip = camera->GetFarClip();
    drawQueue.AddShaderParameter(VSP_NEARCLIP, nearClip);
    drawQueue.AddShaderParameter(VSP_FARCLIP, farClip);
    drawQueue.AddShaderParameter(PSP_NEARCLIP, nearClip);
    drawQueue.AddShaderParameter(PSP_FARCLIP, farClip);

    drawQueue.AddShaderParameter(VSP_DEPTHMODE, GetCameraDepthModeParameter(camera));
    drawQueue.AddShaderParameter(PSP_DEPTHRECONSTRUCT, GetCameraDepthReconstructParameter(camera));

    Vector3 nearVector, farVector;
    camera->GetFrustumSize(nearVector, farVector);
    drawQueue.AddShaderParameter(VSP_FRUSTUMSIZE, farVector);

    drawQueue.AddShaderParameter(VSP_VIEWPROJ, camera->GetEffectiveGPUViewProjection(constantDepthBias));

    drawQueue.AddShaderParameter(PSP_AMBIENTCOLOR, camera->GetEffectiveAmbientColor());
    drawQueue.AddShaderParameter(PSP_FOGCOLOR, camera->GetEffectiveFogColor());
    drawQueue.AddShaderParameter(PSP_FOGPARAMS, GetFogParameter(camera));
}

}

SceneBatchRenderer::SceneBatchRenderer(Context* context)
    : Object(context)
    , graphics_(context_->GetSubsystem<Graphics>())
    , renderer_(context_->GetSubsystem<Renderer>())
{}

void SceneBatchRenderer::RenderUnlitBaseBatches(DrawCommandQueue& drawQueue, const DrawableProcessor& drawableProcessor,
    Camera* camera, Zone* zone, ea::span<const PipelineBatchByState> batches)
{
    //const auto getBatchLight = [&](const PipelineBatchByState& batch) { return nullptr; };
    RenderBatches<false>(drawQueue, drawableProcessor, camera, zone, batches);
}

void SceneBatchRenderer::RenderLitBaseBatches(DrawCommandQueue& drawQueue, const DrawableProcessor& drawableProcessor,
    Camera* camera, Zone* zone, ea::span<const PipelineBatchByState> batches)
{
    /*LightProcessor* mainLight = drawableProcessor.GetMainLight();
    const auto getBatchLight = [&](const PipelineBatchByState& batch)
    {
        return mainLight;
    };*/
    RenderBatches<true>(drawQueue, drawableProcessor, camera, zone, batches);
}

/*void SceneBatchRenderer::RenderLightBatches(DrawCommandQueue& drawQueue, const DrawableProcessor& drawableProcessor,
    Camera* camera, Zone* zone, ea::span<const LightBatchSortedByState> batches)
{
    const ea::vector<LightProcessor*>& visibleLights = drawableProcessor.GetVisibleLights();
    const auto getBatchLight = [&](const LightBatchSortedByState& batch)
    {
        return visibleLights[batch.lightIndex_];
    };
    RenderBatches<true>(drawQueue, drawableProcessor, camera, zone, batches, getBatchLight);
}*/

void SceneBatchRenderer::RenderAlphaBatches(DrawCommandQueue& drawQueue, const DrawableProcessor& drawableProcessor,
    Camera* camera, Zone* zone, ea::span<const PipelineBatchBackToFront> batches)
{
    /*const ea::vector<LightProcessor*>& visibleLights = drawableProcessor.GetVisibleLights();
    const auto getBatchLight = [&](const PipelineBatchBackToFront& batch)
    {
        const unsigned lightIndex = batch.sceneBatch_->lightIndex_;
        return lightIndex != M_MAX_UNSIGNED ? visibleLights[lightIndex] : nullptr;
    };*/
    RenderBatches<true>(drawQueue, drawableProcessor, camera, zone, batches);
}

void SceneBatchRenderer::RenderShadowBatches(DrawCommandQueue& drawQueue, const DrawableProcessor& drawableProcessor,
    Camera* camera, Zone* zone, ea::span<const PipelineBatchByState> batches)
{
    //const auto getBatchLight = [&](const PipelineBatchByState& batch) { return nullptr; };
    RenderBatches<false>(drawQueue, drawableProcessor, camera, zone, batches);
}

void SceneBatchRenderer::RenderLightVolumeBatches(DrawCommandQueue& drawQueue, const DrawableProcessor& drawableProcessor,
    Camera* camera, Zone* zone, ea::span<const LightVolumeBatch> batches,
    ea::span<const GeometryBufferResource> geometryBuffer, const Vector4& geometryBufferOffset, const Vector2& geometryBufferInvSize)
{
    // TODO(renderer): Remove copypaste
    const FrameInfo& frameInfo = drawableProcessor.GetFrameInfo();
    const Scene* scene = frameInfo.octree_->GetScene();
    const ea::vector<LightProcessor*>& visibleLights = drawableProcessor.GetLightProcessors();
    Node* cameraNode = camera->GetNode();

    static const SceneLightShaderParameters defaultLightParams;
    const SceneLightShaderParameters* currentLightParams = &defaultLightParams;
    Texture2D* currentShadowMap = nullptr;

    bool frameDirty = true;
    bool cameraDirty = true;
    float previousConstantDepthBias = 0.0f;
    const LightProcessor* previousLight = nullptr;

    for (const LightVolumeBatch& batch : batches)
    {
        PipelineState* pipelineState = batch.pipelineState_;
        if (!pipelineState)
        {
            static std::atomic_flag errorReported;
            if (!errorReported.test_and_set())
                URHO3D_LOGERROR("Cannot render scene batch witout pipeline state");
            continue;
        }

        // Get used light
        const LightProcessor* light = visibleLights[batch.lightIndex_];
        const bool lightDirty = light != previousLight;
        if (lightDirty)
        {
            previousLight = light;
            currentLightParams = light ? &light->GetShaderParams() : &defaultLightParams;
            currentShadowMap = light ? light->GetShadowMap().texture_ : nullptr;
        }

        // Always set pipeline state first
        drawQueue.SetPipelineState(batch.pipelineState_);

        // Reset camera parameters if depth bias changed
        const float constantDepthBias = pipelineState->GetDesc().constantDepthBias_;

        // Add frame parameters
        if (drawQueue.BeginShaderParameterGroup(SP_FRAME, frameDirty))
        {
            AddFrameShaderParameters(drawQueue, frameInfo, scene);
            drawQueue.CommitShaderParameterGroup(SP_FRAME);
            frameDirty = false;
        }

        // Add camera parameters
        if (drawQueue.BeginShaderParameterGroup(SP_CAMERA,
            !cameraDirty || previousConstantDepthBias != constantDepthBias))
        {
            AddCameraShaderParameters(drawQueue, camera, constantDepthBias);
            drawQueue.AddShaderParameter(VSP_GBUFFEROFFSETS, geometryBufferOffset);
            drawQueue.AddShaderParameter(PSP_GBUFFERINVSIZE, geometryBufferInvSize);
            drawQueue.CommitShaderParameterGroup(SP_CAMERA);
            cameraDirty = false;
            previousConstantDepthBias = constantDepthBias;
        }

        // Add light parameters
        if constexpr (1)
        {
            if (drawQueue.BeginShaderParameterGroup(SP_LIGHT, lightDirty))
            {
                drawQueue.AddShaderParameter(VSP_LIGHTDIR, currentLightParams->direction_);
                drawQueue.AddShaderParameter(VSP_LIGHTPOS,
                    Vector4{ currentLightParams->position_, currentLightParams->invRange_ });
                drawQueue.AddShaderParameter(PSP_LIGHTCOLOR,
                    Vector4{ currentLightParams->color_, currentLightParams->specularIntensity_ });
                drawQueue.AddShaderParameter(PSP_LIGHTRAD, currentLightParams->radius_);
                drawQueue.AddShaderParameter(PSP_LIGHTLENGTH, currentLightParams->length_);

                if (currentLightParams->numLightMatrices_ > 0)
                {
                    ea::span<const Matrix4> shadowMatricesSpan{ currentLightParams->lightMatrices_, currentLightParams->numLightMatrices_ };
                    drawQueue.AddShaderParameter(VSP_LIGHTMATRICES, shadowMatricesSpan);
                }
                if (currentShadowMap)
                {
                    drawQueue.AddShaderParameter(PSP_SHADOWDEPTHFADE, currentLightParams->shadowDepthFade_);
                    drawQueue.AddShaderParameter(PSP_SHADOWINTENSITY, currentLightParams->shadowIntensity_);
                    drawQueue.AddShaderParameter(PSP_SHADOWMAPINVSIZE, currentLightParams->shadowMapInvSize_);
                    drawQueue.AddShaderParameter(PSP_SHADOWSPLITS, currentLightParams->shadowSplits_);
                    drawQueue.AddShaderParameter(PSP_SHADOWCUBEUVBIAS, currentLightParams->shadowCubeUVBias_);
                    drawQueue.AddShaderParameter(PSP_SHADOWCUBEADJUST, currentLightParams->shadowCubeAdjust_);
                    drawQueue.AddShaderParameter(VSP_NORMALOFFSETSCALE, currentLightParams->normalOffsetScale_);
                    drawQueue.AddShaderParameter(PSP_VSMSHADOWPARAMS, renderer_->GetVSMShadowParameters());
                }

                drawQueue.CommitShaderParameterGroup(SP_LIGHT);
            }
        }

        // Add resources
        if (lightDirty)
        {
            for (const auto& texture : geometryBuffer)
                drawQueue.AddShaderResource(texture.unit_, texture.texture_);

            drawQueue.AddShaderResource(TU_LIGHTRAMP, renderer_->GetDefaultLightRamp());
            drawQueue.AddShaderResource(TU_LIGHTSHAPE, renderer_->GetDefaultLightSpot());
            if (currentShadowMap)
                drawQueue.AddShaderResource(TU_SHADOWMAP, currentShadowMap);
            drawQueue.CommitShaderResources();
        }

        // Add object parameters
        if (drawQueue.BeginShaderParameterGroup(SP_OBJECT, true))
        {
            drawQueue.AddShaderParameter(VSP_MODEL, light->GetLight()->GetVolumeTransform(camera));
            drawQueue.CommitShaderParameterGroup(SP_OBJECT);
        }

        // Set buffers and draw
        drawQueue.SetBuffers(batch.geometry_->GetVertexBuffers(), batch.geometry_->GetIndexBuffer());
        drawQueue.DrawIndexed(batch.geometry_->GetIndexStart(), batch.geometry_->GetIndexCount());
    }
}

template <bool HasLight, class BatchType>
void SceneBatchRenderer::RenderBatches(DrawCommandQueue& drawQueue, const DrawableProcessor& drawableProcessor,
    Camera* camera, Zone* zone, ea::span<const BatchType> batches)
{
    const FrameInfo& frameInfo = drawableProcessor.GetFrameInfo();
    const Scene* scene = frameInfo.octree_->GetScene();
    const ea::vector<LightProcessor*>& visibleLights = drawableProcessor.GetLightProcessors();
    Node* cameraNode = camera->GetNode();

    static const SceneLightShaderParameters defaultLightParams;
    const SceneLightShaderParameters* currentLightParams = &defaultLightParams;
    Texture2D* currentShadowMap = nullptr;

    bool frameDirty = true;
    bool cameraDirty = true;
    float previousConstantDepthBias = 0.0f;
    const LightProcessor* previousLight = nullptr;
    LightAccumulator::VertexLightContainer previousVertexLights;
    Material* previousMaterial = nullptr;

    for (const auto& sortedBatch : batches)
    {
        const PipelineBatch& batch = *sortedBatch.sceneBatch_;
        const SourceBatch& sourceBatch = batch.GetSourceBatch();
        PipelineState* pipelineState = batch.pipelineState_;
        if (!pipelineState)
        {
            static std::atomic_flag errorReported;
            if (!errorReported.test_and_set())
                URHO3D_LOGERROR("Cannot render scene batch witout pipeline state");
            continue;
        }

        // Get used light
        const LightProcessor* light = batch.lightIndex_ != M_MAX_UNSIGNED ? visibleLights[batch.lightIndex_] : nullptr;
        const bool lightDirty = light != previousLight;
        if (lightDirty)
        {
            previousLight = light;
            currentLightParams = light ? &light->GetShaderParams() : &defaultLightParams;
            currentShadowMap = light ? light->GetShadowMap().texture_ : nullptr;
        }

        // Always set pipeline state first
        drawQueue.SetPipelineState(batch.pipelineState_);

        // Reset camera parameters if depth bias changed
        const float constantDepthBias = pipelineState->GetDesc().constantDepthBias_;

        // Add frame parameters
        if (drawQueue.BeginShaderParameterGroup(SP_FRAME, frameDirty))
        {
            AddFrameShaderParameters(drawQueue, frameInfo, scene);
            drawQueue.CommitShaderParameterGroup(SP_FRAME);
            frameDirty = false;
        }

        // Add camera parameters
        if (drawQueue.BeginShaderParameterGroup(SP_CAMERA,
            !cameraDirty || previousConstantDepthBias != constantDepthBias))
        {
            AddCameraShaderParameters(drawQueue, camera, constantDepthBias);
            drawQueue.CommitShaderParameterGroup(SP_CAMERA);
            cameraDirty = false;
            previousConstantDepthBias = constantDepthBias;
        }

        // Add light parameters
        if constexpr (HasLight)
        {
            const LightAccumulator& lighting = drawableProcessor.GetGeometryLighting(batch.drawableIndex_);
            auto vertexLights = lighting.GetVertexLights();
            ea::sort(vertexLights.begin(), vertexLights.end());
            const bool vertexLightsDirty = previousVertexLights != vertexLights;
            if (drawQueue.BeginShaderParameterGroup(SP_LIGHT, lightDirty || vertexLightsDirty))
            {
                previousVertexLights = vertexLights;

                drawQueue.AddShaderParameter(VSP_LIGHTDIR, currentLightParams->direction_);
                drawQueue.AddShaderParameter(VSP_LIGHTPOS,
                    Vector4{ currentLightParams->position_, currentLightParams->invRange_ });
                drawQueue.AddShaderParameter(PSP_LIGHTCOLOR,
                    Vector4{ currentLightParams->color_, currentLightParams->specularIntensity_ });

                drawQueue.AddShaderParameter(PSP_LIGHTRAD, currentLightParams->radius_);
                drawQueue.AddShaderParameter(PSP_LIGHTLENGTH, currentLightParams->length_);

                Vector4 vertexLightsData[MAX_VERTEX_LIGHTS * 3]{};
                for (unsigned i = 0; i < vertexLights.size(); ++i)
                {
                    if (vertexLights[i] == M_MAX_UNSIGNED)
                        continue;

                    const SceneLightShaderParameters& vertexLightParams = visibleLights[vertexLights[i]]->GetShaderParams();
                    vertexLightsData[i * 3] = { vertexLightParams.color_, vertexLightParams.invRange_ };
                    vertexLightsData[i * 3 + 1] = { vertexLightParams.direction_, vertexLightParams.cutoff_ };
                    vertexLightsData[i * 3 + 2] = { vertexLightParams.position_, vertexLightParams.invCutoff_ };
                }
                drawQueue.AddShaderParameter(VSP_VERTEXLIGHTS, ea::span<const Vector4>{ vertexLightsData });

                if (currentLightParams->numLightMatrices_ > 0)
                {
                    ea::span<const Matrix4> shadowMatricesSpan{ currentLightParams->lightMatrices_, currentLightParams->numLightMatrices_ };
                    drawQueue.AddShaderParameter(VSP_LIGHTMATRICES, shadowMatricesSpan);
                }
                if (currentShadowMap)
                {
                    drawQueue.AddShaderParameter(PSP_SHADOWDEPTHFADE, currentLightParams->shadowDepthFade_);
                    drawQueue.AddShaderParameter(PSP_SHADOWINTENSITY, currentLightParams->shadowIntensity_);
                    drawQueue.AddShaderParameter(PSP_SHADOWMAPINVSIZE, currentLightParams->shadowMapInvSize_);
                    drawQueue.AddShaderParameter(PSP_SHADOWSPLITS, currentLightParams->shadowSplits_);
                    drawQueue.AddShaderParameter(PSP_SHADOWCUBEUVBIAS, currentLightParams->shadowCubeUVBias_);
                    drawQueue.AddShaderParameter(PSP_SHADOWCUBEADJUST, currentLightParams->shadowCubeAdjust_);
                    drawQueue.AddShaderParameter(VSP_NORMALOFFSETSCALE, currentLightParams->normalOffsetScale_);
                    drawQueue.AddShaderParameter(PSP_VSMSHADOWPARAMS, renderer_->GetVSMShadowParameters());
                }

                drawQueue.CommitShaderParameterGroup(SP_LIGHT);
            }
        }

        // Add material parameters
        const bool materialDirty = previousMaterial != batch.material_;
        if (drawQueue.BeginShaderParameterGroup(SP_MATERIAL, materialDirty))
        {
            const auto& materialParameters = batch.material_->GetShaderParameters();
            for (const auto& parameter : materialParameters)
                drawQueue.AddShaderParameter(parameter.first, parameter.second.value_);
            drawQueue.CommitShaderParameterGroup(SP_MATERIAL);
        }

        // Add resources
        if (materialDirty || lightDirty)
        {
            const auto& materialTextures = batch.material_->GetTextures();
            for (const auto& texture : materialTextures)
                drawQueue.AddShaderResource(texture.first, texture.second);

            drawQueue.AddShaderResource(TU_LIGHTRAMP, renderer_->GetDefaultLightRamp());
            drawQueue.AddShaderResource(TU_LIGHTSHAPE, renderer_->GetDefaultLightSpot());
            if (currentShadowMap)
                drawQueue.AddShaderResource(TU_SHADOWMAP, currentShadowMap);
            drawQueue.CommitShaderResources();

            previousMaterial = batch.material_;
        }

        // Add object parameters
        if (drawQueue.BeginShaderParameterGroup(SP_OBJECT, true))
        {
            const LightAccumulator& lighting = drawableProcessor.GetGeometryLighting(batch.drawableIndex_);
            const SphericalHarmonicsDot9& sh = lighting.sh_;
            drawQueue.AddShaderParameter(VSP_SHAR, sh.Ar_);
            drawQueue.AddShaderParameter(VSP_SHAG, sh.Ag_);
            drawQueue.AddShaderParameter(VSP_SHAB, sh.Ab_);
            drawQueue.AddShaderParameter(VSP_SHBR, sh.Br_);
            drawQueue.AddShaderParameter(VSP_SHBG, sh.Bg_);
            drawQueue.AddShaderParameter(VSP_SHBB, sh.Bb_);
            drawQueue.AddShaderParameter(VSP_SHC, sh.C_);
            drawQueue.AddShaderParameter(VSP_AMBIENT, Vector4(sh.EvaluateAverage(), 1.0f));
            switch (batch.geometryType_)
            {
            case GEOM_INSTANCED:
                // TODO(renderer): Implement instancing
                assert(0);
                break;
            case GEOM_SKINNED:
                drawQueue.AddShaderParameter(VSP_SKINMATRICES,
                    ea::span<const Matrix3x4>(sourceBatch.worldTransform_, sourceBatch.numWorldTransforms_));
                break;
            case GEOM_BILLBOARD:
                drawQueue.AddShaderParameter(VSP_MODEL, *sourceBatch.worldTransform_);
                if (sourceBatch.numWorldTransforms_ > 1)
                    drawQueue.AddShaderParameter(VSP_BILLBOARDROT, sourceBatch.worldTransform_[1].RotationMatrix());
                else
                    drawQueue.AddShaderParameter(VSP_BILLBOARDROT, cameraNode->GetWorldRotation().RotationMatrix());
                break;
            default:
                drawQueue.AddShaderParameter(VSP_MODEL, *sourceBatch.worldTransform_);
                break;
            }
            drawQueue.CommitShaderParameterGroup(SP_OBJECT);
        }

        // Set buffers and draw
        drawQueue.SetBuffers(sourceBatch.geometry_->GetVertexBuffers(), sourceBatch.geometry_->GetIndexBuffer());
        drawQueue.DrawIndexed(sourceBatch.geometry_->GetIndexStart(), sourceBatch.geometry_->GetIndexCount());
    }
}

}
