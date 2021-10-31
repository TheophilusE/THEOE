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
#include "../Core/Exception.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Light.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/ModelView.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Zone.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../Resource/BinaryFile.h"
#include "../Resource/Image.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Scene.h"
#include "../Utility/GLTFImporter.h"

#include <tiny_gltf.h>

#include <EASTL/numeric.h>
#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>
#include <EASTL/unordered_map.h>

#include <exception>

#include "../DebugNew.h"

namespace Urho3D
{

namespace tg = tinygltf;

namespace
{

const unsigned MaxNameAssignTries = 64*1024;

template <class T>
struct StaticCaster
{
    template <class U>
    T operator() (U x) const { return static_cast<T>(x); }
};

template <class T, unsigned N, class U>
ea::array<T, N> ToArray(const U& vec)
{
    ea::array<T, N> result{};
    if (vec.size() >= N)
        ea::transform(vec.begin(), vec.begin() + N, result.begin(), StaticCaster<T>{});
    return result;
}

bool IsNegativeScale(const Vector3& scale) { return scale.x_ * scale.y_ * scale.y_ < 0.0f; }

Vector3 MirrorX(const Vector3& vec) { return { -vec.x_, vec.y_, vec.z_ }; }

Quaternion MirrorX(const Quaternion& rotation)
{
    Matrix3 mat = rotation.RotationMatrix();
    mat.m01_ = -mat.m01_;
    mat.m10_ = -mat.m10_;
    mat.m02_ = -mat.m02_;
    mat.m20_ = -mat.m20_;
    return Quaternion{ mat };
}

Matrix3x4 MirrorX(Matrix3x4 mat)
{
    mat.m01_ = -mat.m01_;
    mat.m10_ = -mat.m10_;
    mat.m02_ = -mat.m02_;
    mat.m20_ = -mat.m20_;
    mat.m03_ = -mat.m03_;
    return mat;
}

/// Raw imported input, parameters and generic output layout.
class GLTFImporterBase : public NonCopyable
{
public:
    GLTFImporterBase(Context* context, tg::Model model,
        const ea::string& outputPath, const ea::string& resourceNamePrefix)
        : context_(context)
        , model_(ea::move(model))
        , outputPath_(outputPath)
        , resourceNamePrefix_(resourceNamePrefix)
    {
    }

    ea::string CreateLocalResourceName(const ea::string& nameHint,
        const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        const ea::string body = !nameHint.empty() ? nameHint : defaultName;
        for (unsigned i = 0; i < MaxNameAssignTries; ++i)
        {
            const ea::string_view nameFormat = i != 0 ? "{0}{1}_{2}{3}" : "{0}{1}{3}";
            const ea::string localResourceName = Format(nameFormat, prefix, body, i, suffix);
            if (localResourceNames_.contains(localResourceName))
                continue;

            localResourceNames_.emplace(localResourceName);
            return localResourceName;
        }

        // Should never happen
        throw RuntimeException("Cannot assign resource name");
    }

    ea::string CreateResourceName(const ea::string& localResourceName)
    {
        const ea::string resourceName = resourceNamePrefix_ + localResourceName;
        const ea::string absoluteFileName = outputPath_ + localResourceName;
        resourceNameToAbsoluteFileName_[resourceName] = absoluteFileName;
        return resourceName;
    }

    ea::string GetResourceName(const ea::string& nameHint,
        const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        const ea::string localResourceName = CreateLocalResourceName(nameHint, prefix, defaultName, suffix);
        return CreateResourceName(localResourceName);
    }

    const ea::string& GetAbsoluteFileName(const ea::string& resourceName)
    {
        const auto iter = resourceNameToAbsoluteFileName_.find(resourceName);
        return iter != resourceNameToAbsoluteFileName_.end() ? iter->second : EMPTY_STRING;
    }

    void AddToResourceCache(Resource* resource)
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        cache->AddManualResource(resource);
    }

    void SaveResource(Resource* resource)
    {
        const ea::string& fileName = GetAbsoluteFileName(resource->GetName());
        if (fileName.empty())
            throw RuntimeException("Cannot save imported resource");
        resource->SaveFile(fileName);
    }

    void SaveResource(Scene* scene)
    {
        XMLFile xmlFile(scene->GetContext());
        XMLElement rootElement = xmlFile.GetOrCreateRoot("scene");
        scene->SaveXML(rootElement);
        xmlFile.SaveFile(scene->GetFileName());
    }

    const tg::Model& GetModel() const { return model_; }
    Context* GetContext() const { return context_; }

    void CheckAccessor(int index) const { CheckT(index, model_.accessors, "Invalid accessor #{} referenced"); }
    void CheckBufferView(int index) const { CheckT(index, model_.bufferViews, "Invalid buffer view #{} referenced"); }
    void CheckImage(int index) const { CheckT(index, model_.images, "Invalid image #{} referenced"); }
    void CheckMaterial(int index) const { CheckT(index, model_.materials, "Invalid material #{} referenced"); }
    void CheckMesh(int index) const { CheckT(index, model_.meshes, "Invalid mesh #{} referenced"); }
    void CheckNode(int index) const { CheckT(index, model_.nodes, "Invalid node #{} referenced"); }
    void CheckSampler(int index) const { CheckT(index, model_.samplers, "Invalid sampler #{} referenced"); }
    void CheckSkin(int index) const { CheckT(index, model_.skins, "Invalid skin #{} referenced"); }

private:
    template <class T>
    void CheckT(int index, const T& container, const char* message) const
    {
        if (index < 0 || index >= container.size())
            throw RuntimeException(message, index);
    }

    Context* const context_{};
    const tg::Model model_;
    const ea::string outputPath_;
    const ea::string resourceNamePrefix_;

    ea::unordered_set<ea::string> localResourceNames_;
    ea::unordered_map<ea::string, ea::string> resourceNameToAbsoluteFileName_;
};

/// Utility to parse GLTF buffers.
class GLTFBufferReader : public NonCopyable
{
public:
    explicit GLTFBufferReader(GLTFImporterBase& base)
        : base_(base)
        , model_(base_.GetModel())
    {
    }

    template <class T>
    ea::vector<T> ReadBufferView(int bufferViewIndex, int byteOffset, int componentType, int type, int count) const
    {
        base_.CheckBufferView(bufferViewIndex);

        const int numComponents = tg::GetNumComponentsInType(type);
        if (numComponents <= 0)
            throw RuntimeException("Unexpected type {} of buffer view elements", type);

        const tg::BufferView& bufferView = model_.bufferViews[bufferViewIndex];

        ea::vector<T> result(count * numComponents);
        switch (componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
            ReadBufferViewImpl<signed char>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            ReadBufferViewImpl<unsigned char>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_SHORT:
            ReadBufferViewImpl<short>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            ReadBufferViewImpl<unsigned short>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_INT:
            ReadBufferViewImpl<int>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            ReadBufferViewImpl<unsigned>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            ReadBufferViewImpl<float>(result, bufferView, byteOffset, componentType, type, count);
            break;

        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            ReadBufferViewImpl<double>(result, bufferView, byteOffset, componentType, type, count);
            break;

        default:
            throw RuntimeException("Unsupported component type {} of buffer view elements", componentType);
        }

        return result;
    }

    template <class T>
    ea::vector<T> ReadAccessorChecked(const tg::Accessor& accessor) const
    {
        const auto result = ReadAccessor<T>(accessor);
        if (result.size() != accessor.count)
            throw RuntimeException("Unexpected number of objects in accessor");
        return result;
    }

    template <class T>
    ea::vector<T> ReadAccessor(const tg::Accessor& accessor) const
    {
        const int numComponents = tg::GetNumComponentsInType(accessor.type);
        if (numComponents <= 0)
            throw RuntimeException("Unexpected type {} of buffer view elements", accessor.type);

        // Read dense buffer data
        ea::vector<T> result;
        if (accessor.bufferView >= 0)
        {
            result = ReadBufferView<T>(accessor.bufferView, accessor.byteOffset,
                accessor.componentType, accessor.type, accessor.count);
        }
        else
        {
            result.resize(accessor.count * numComponents);
        }

        // Read sparse buffer data
        const int numSparseElements = accessor.sparse.count;
        if (accessor.sparse.isSparse && numSparseElements > 0)
        {
            const auto& accessorIndices = accessor.sparse.indices;
            const auto& accessorValues = accessor.sparse.values;

            const auto indices = ReadBufferView<unsigned>(accessorIndices.bufferView, accessorIndices.byteOffset,
                accessorIndices.componentType, TINYGLTF_TYPE_SCALAR, numSparseElements);

            const auto values = ReadBufferView<T>(accessorValues.bufferView, accessorValues.byteOffset,
                accessor.componentType, accessor.type, numSparseElements);

            for (unsigned i = 0; i < indices.size(); ++i)
                ea::copy_n(&values[i * numComponents], numComponents, &result[indices[i] * numComponents]);
        }

        return result;
    }

private:
    static int GetByteStride(const tg::BufferView& bufferViewObject, int componentType, int type)
    {
        const int componentSizeInBytes = tg::GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
        const int numComponents = tg::GetNumComponentsInType(static_cast<uint32_t>(type));
        if (componentSizeInBytes <= 0 || numComponents <= 0)
            return -1;

        return bufferViewObject.byteStride == 0
            ? componentSizeInBytes * numComponents
            : static_cast<int>(bufferViewObject.byteStride);
    }

    template <class T, class U>
    void ReadBufferViewImpl(ea::vector<U>& result,
        const tg::BufferView& bufferView, int byteOffset, int componentType, int type, int count) const
    {
        const tg::Buffer& buffer = model_.buffers[bufferView.buffer];

        const auto* bufferViewData = buffer.data.data() + bufferView.byteOffset + byteOffset;
        const int stride = GetByteStride(bufferView, componentType, type);

        const int numComponents = tg::GetNumComponentsInType(type);
        for (unsigned i = 0; i < count; ++i)
        {
            for (unsigned j = 0; j < numComponents; ++j)
            {
                T elementValue{};
                memcpy(&elementValue, bufferViewData + sizeof(T) * j, sizeof(T));
                result[i * numComponents + j] = static_cast<U>(elementValue);
            }
            bufferViewData += stride;
        }
    }

    template <class T>
    static ea::vector<T> RepackFloats(const ea::vector<float>& source)
    {
        static constexpr unsigned numComponents = sizeof(T) / sizeof(float);
        if (source.size() % numComponents != 0)
            throw RuntimeException("Unexpected number of components in array");

        const unsigned numElements = source.size() / numComponents;

        ea::vector<T> result;
        result.resize(numElements);
        for (unsigned i = 0; i < numElements; ++i)
            std::memcpy(&result[i], &source[i * numComponents], sizeof(T));
        return result;
    }

    const GLTFImporterBase& base_;
    const tg::Model& model_;
};

template <>
ea::vector<Vector2> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Vector2>(ReadAccessor<float>(accessor)); }

template <>
ea::vector<Vector3> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Vector3>(ReadAccessor<float>(accessor)); }

template <>
ea::vector<Vector4> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Vector4>(ReadAccessor<float>(accessor)); }

template <>
ea::vector<Matrix4> GLTFBufferReader::ReadAccessor(const tg::Accessor& accessor) const { return RepackFloats<Matrix4>(ReadAccessor<float>(accessor)); }

/// GLTF node reference used for hierarchy view.
struct GLTFNode;
using GLTFNodePtr = ea::shared_ptr<GLTFNode>;

struct GLTFNode : public ea::enable_shared_from_this<GLTFNode>
{
    /// Hierarchy metadata
    /// @{
    GLTFNode* root_{};
    GLTFNode* parent_{};
    ea::vector<GLTFNodePtr> children_;
    /// @}

    /// Data directly imported from GLTF
    /// @{
    unsigned index_{};
    ea::string name_;

    Vector3 position_;
    Quaternion rotation_;
    Vector3 scale_{ Vector3::ONE };

    ea::optional<unsigned> mesh_;
    ea::optional<unsigned> skin_;

    ea::vector<unsigned> containedInSkins_;
    /// @}

    /// Data generated by importer
    /// @{
    ea::optional<unsigned> skeletonIndex_;
    ea::optional<ea::string> uniqueBoneName_;
    ea::vector<unsigned> skinnedMeshNodes_;
    /// @}
};

/// Represents Urho skeleton which may be composed from one or more GLTF skins.
/// Does *not* contain mesh-related specifics like bone indices and bind matrices.
struct GLTFSkeleton
{
    unsigned index_{};
    ea::vector<unsigned> skins_;
    GLTFNode* rootNode_{};
    ea::unordered_map<ea::string, GLTFNode*> boneNameToNode_;
};

/// Represents GLTF skin as Urho skeleton with bone indices and bind matrices.
struct GLTFSkin
{
    unsigned index_{};
    const GLTFSkeleton* skeleton_{};

    /// Bone nodes, contain exactly one root node.
    ea::vector<const GLTFNode*> boneNodes_;
    ea::vector<Matrix3x4> inverseBindMatrices_;
    /// Note: Does *not* contain bounding volumes.
    ea::vector<BoneView> cookedBones_;
};

/// Represents unique Urho model with optional skin.
struct GLTFMeshSkinPair
{
    unsigned mesh_{};
    ea::optional<unsigned> skin_;
};
using GLTFMeshSkinPairPtr = ea::shared_ptr<GLTFMeshSkinPair>;

/// Utility to process scene and node hierarchy of source GLTF asset.
/// Mirrors the scene to convert from right-handed to left-handed coordinates.
///
/// Implements simple heuristics: if no models are actually mirrored after initial mirror,
/// then GLTF exporter implemented lazy export from left-handed to right-handed coordinate system
/// by adding top-level mirroring. In this case, keep scene as is.
/// TODO: Cleanup redundant mirrors?
/// Otherwise scene is truly left-handed and deep mirroring is needed.
///
/// Converts skins to the format consumable by Urho scene.
class GLTFHierarchyAnalyzer : public NonCopyable
{
public:
    explicit GLTFHierarchyAnalyzer(GLTFImporterBase& base, const GLTFBufferReader& bufferReader)
        : base_(base)
        , bufferReader_(bufferReader)
        , model_(base_.GetModel())
    {
        InitializeParents();
        InitializeTrees();
        ConvertToLeftHandedCoordinates();
        PreProcessSkins();
        InitializeSkeletons();
        InitializeSkins();
        AssignSkinnedModelsToNodes();
        EnumerateUniqueMeshSkinPairs();
    }

    bool IsDeepMirrored() const { return isDeepMirrored_; }

    const GLTFNode& GetNode(int nodeIndex) const
    {
        base_.CheckNode(nodeIndex);
        return *nodeToTreeNode_[nodeIndex];
    }

    ea::string GetEffectiveNodeName(const GLTFNode& node) const
    {
        if (!node.skinnedMeshNodes_.empty())
        {
            ea::string name;
            for (int meshNodeIndex : node.skinnedMeshNodes_)
            {
                base_.CheckNode(meshNodeIndex);
                const GLTFNode& meshNode = *nodeToTreeNode_[meshNodeIndex];
                if (!meshNode.name_.empty())
                {
                    if (!name.empty())
                        name += '_';
                    name += meshNode.name_;
                }
            }
            if (name.empty())
                name = "SkinnedMesh";
            return name;
        }

        if (node.uniqueBoneName_)
            return *node.uniqueBoneName_;

        return node.name_;
    }

    const ea::vector<GLTFMeshSkinPairPtr>& GetUniqueMeshSkinPairs() const { return uniqueMeshSkinPairs_; }

    unsigned GetUniqueMeshSkin(int meshIndex, int skinIndex) const
    {
        const auto key = ea::make_pair(meshIndex, skinIndex);
        const auto iter = meshSkinPairs_.find(key);
        if (iter == meshSkinPairs_.end())
            throw RuntimeException("Cannot find mesh #{} with skin #{}", meshIndex, skinIndex);
        return iter->second;
    }

    const ea::vector<BoneView>& GetSkinBones(ea::optional<unsigned> skinIndex) const
    {
        static const ea::vector<BoneView> emptyBones;
        if (!skinIndex)
            return emptyBones;

        base_.CheckSkin(*skinIndex);
        return skins_[*skinIndex].cookedBones_;
    }

    const GLTFSkeleton& GetSkeleton(unsigned skeletonIndex) const
    {
        if (skeletonIndex >= skeletons_.size())
            throw RuntimeException("Invalid skeleton #{} is referenced", skeletonIndex);
        return skeletons_[skeletonIndex];
    }

private:
    void InitializeParents()
    {
        const unsigned numNodes = model_.nodes.size();
        nodeToParent_.resize(numNodes);
        for (unsigned nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
        {
            const tg::Node& node = model_.nodes[nodeIndex];
            for (const int childIndex : node.children)
            {
                base_.CheckNode(childIndex);

                if (nodeToParent_[childIndex].has_value())
                {
                    throw RuntimeException("Node #{} has multiple parents: #{} and #{}",
                        childIndex, nodeIndex, *nodeToParent_[childIndex]);
                }

                nodeToParent_[childIndex] = nodeIndex;
            }
        }
    }

    void InitializeTrees()
    {
        const unsigned numNodes = model_.nodes.size();
        nodeToTreeNode_.resize(numNodes);
        for (unsigned nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
        {
            if (!nodeToParent_[nodeIndex])
                trees_.push_back(ImportTree(nodeIndex, nullptr, nullptr));
        }

        for (const GLTFNodePtr& node : trees_)
            ReadNodeProperties(*node);
    }

    GLTFNodePtr ImportTree(unsigned nodeIndex, GLTFNode* parent, GLTFNode* root)
    {
        base_.CheckNode(nodeIndex);
        const tg::Node& sourceNode = model_.nodes[nodeIndex];

        auto node = ea::make_shared<GLTFNode>();
        root = root ? root : node.get();

        node->index_ = nodeIndex;
        node->root_ = root;
        node->parent_ = parent;
        for (const int childIndex : sourceNode.children)
            node->children_.push_back(ImportTree(childIndex, node.get(), root));

        nodeToTreeNode_[nodeIndex] = node.get();
        return node;
    }

    void ReadNodeProperties(GLTFNode& node) const
    {
        const tg::Node& sourceNode = model_.nodes[node.index_];
        node.name_ = sourceNode.name.c_str();

        if (sourceNode.mesh >= 0)
        {
            base_.CheckMesh(sourceNode.mesh);
            node.mesh_ = sourceNode.mesh;
        }

        if (sourceNode.skin >= 0)
        {
            base_.CheckSkin(sourceNode.skin);
            node.skin_ = sourceNode.skin;
        }

        if (!sourceNode.matrix.empty())
        {
            const Matrix3x4 matrix = ReadMatrix3x4(sourceNode.matrix);
            matrix.Decompose(node.position_, node.rotation_, node.scale_);
        }
        else
        {
            if (!sourceNode.translation.empty())
                node.position_ = ReadVector3(sourceNode.translation);
            if (!sourceNode.rotation.empty())
                node.rotation_ = ReadQuaternion(sourceNode.rotation);
            if (!sourceNode.scale.empty())
                node.scale_ = ReadVector3(sourceNode.scale);
        }

        for (const GLTFNodePtr& child : node.children_)
            ReadNodeProperties(*child);
    }

    void ConvertToLeftHandedCoordinates()
    {
        isDeepMirrored_ = HasMirroredMeshes(trees_, true);
        if (!isDeepMirrored_)
        {
            for (const GLTFNodePtr& node : trees_)
            {
                node->position_ = MirrorX(node->position_);
                node->rotation_ = MirrorX(node->rotation_);
                node->scale_ = MirrorX(node->scale_);
            }
        }
        else
        {
            for (const GLTFNodePtr& node : trees_)
                DeepMirror(*node);
        }
    }

    bool HasMirroredMeshes(const ea::vector<GLTFNodePtr>& nodes, bool isParentMirrored) const
    {
        return ea::any_of(nodes.begin(), nodes.end(),
            [&](const GLTFNodePtr& node) { return HasMirroredMeshes(*node, isParentMirrored); });
    }

    bool HasMirroredMeshes(GLTFNode& node, bool isParentMirrored) const
    {
        const tg::Node& sourceNode = model_.nodes[node.index_];
        const bool hasMesh = sourceNode.mesh >= 0;
        const bool isMirroredLocal = IsNegativeScale(node.scale_);
        const bool isMirroredWorld = (isParentMirrored != isMirroredLocal);
        if (isMirroredWorld && hasMesh)
            return true;

        return HasMirroredMeshes(node.children_, isMirroredWorld);
    }

    void DeepMirror(GLTFNode& node) const
    {
        node.position_ = MirrorX(node.position_);
        node.rotation_ = MirrorX(node.rotation_);
        for (const GLTFNodePtr& node : node.children_)
            DeepMirror(*node);
    }

    void PreProcessSkins()
    {
        const unsigned numSkins = model_.skins.size();
        skinToRootNode_.resize(numSkins);
        for (unsigned skinIndex = 0; skinIndex < numSkins; ++skinIndex)
        {
            const tg::Skin& sourceSkin = model_.skins[skinIndex];
            GLTFNode& rootNode = GetSkinRoot(sourceSkin);

            MarkInSkin(rootNode, skinIndex);
            for (const int jointNodeIndex : sourceSkin.joints)
            {
                base_.CheckNode(jointNodeIndex);
                GLTFNode& jointNode = *nodeToTreeNode_[jointNodeIndex];

                ForEachInPathExceptParent(jointNode, rootNode,
                    [&](GLTFNode& node) { MarkInSkin(node, skinIndex); });
            }
            skinToRootNode_[skinIndex] = &rootNode;
        }
    }

    GLTFNode& GetSkinRoot(const tg::Skin& sourceSkin) const
    {
        if (sourceSkin.skeleton >= 0)
        {
            base_.CheckNode(sourceSkin.skeleton);
            GLTFNode& skeletonNode = *nodeToTreeNode_[sourceSkin.skeleton];

            for (int nodeIndex : sourceSkin.joints)
            {
                base_.CheckNode(nodeIndex);
                GLTFNode& node = *nodeToTreeNode_[nodeIndex];
                if (!IsChildOf(node, skeletonNode) && &node != &skeletonNode)
                    throw RuntimeException("Skeleton node #{} is not a parent of joint node #{}", sourceSkin.skeleton, nodeIndex);
            }

            return skeletonNode;
        }
        else
        {
            GLTFNode* rootNode = nullptr;
            for (int nodeIndex : sourceSkin.joints)
            {
                base_.CheckNode(nodeIndex);
                GLTFNode& node = *nodeToTreeNode_[nodeIndex];

                if (!rootNode)
                    rootNode = nodeToTreeNode_[nodeIndex];
                else
                {
                    rootNode = GetCommonParent(*rootNode, node);
                    if (!rootNode)
                        throw RuntimeException("Skin doesn't have common root node");
                }
            }

            if (!rootNode)
                throw RuntimeException("Skin doesn't have joints");

            return *rootNode;
        }
    }

    void InitializeSkeletons()
    {
        const unsigned numSkins = model_.skins.size();
        ea::vector<unsigned> skinToGroup(numSkins);
        ea::iota(skinToGroup.begin(), skinToGroup.end(), 0);

        ForEach(trees_, [&](const GLTFNode& child)
        {
            if (child.containedInSkins_.size() <= 1)
                return;

            const unsigned newGroup = skinToGroup[child.containedInSkins_[0]];
            for (unsigned i = 1; i < child.containedInSkins_.size(); ++i)
            {
                const unsigned oldGroup = skinToGroup[child.containedInSkins_[i]];
                if (oldGroup != newGroup)
                    ea::replace(skinToGroup.begin(), skinToGroup.end(), oldGroup, newGroup);
            }
        });

        auto uniqueGroups = skinToGroup;
        ea::sort(uniqueGroups.begin(), uniqueGroups.end());
        uniqueGroups.erase(ea::unique(uniqueGroups.begin(), uniqueGroups.end()), uniqueGroups.end());

        const unsigned numSkeletons = uniqueGroups.size();
        skeletons_.resize(numSkeletons);
        skinToSkeleton_.resize(numSkins);
        for (unsigned skeletonIndex = 0; skeletonIndex < numSkeletons; ++skeletonIndex)
        {
            GLTFSkeleton& skeleton = skeletons_[skeletonIndex];
            for (unsigned skinIndex = 0; skinIndex < numSkins; ++skinIndex)
            {
                if (skinToGroup[skinIndex] == uniqueGroups[skeletonIndex])
                {
                    skeleton.skins_.push_back(skinIndex);
                    skinToSkeleton_[skinIndex] = skeletonIndex;
                }
            }
            if (skeleton.skins_.empty())
                throw RuntimeException("Skeleton must contain at least one skin");
        }

        AssignNodesToSkeletons();

        for (unsigned skeletonIndex = 0; skeletonIndex < skeletons_.size(); ++skeletonIndex)
        {
            GLTFSkeleton& skeleton = skeletons_[skeletonIndex];
            skeleton.index_ = skeletonIndex;
            InitializeSkeletonRootNode(skeleton);
            AssignSkeletonBoneNames(skeleton);
        }
    }

    void AssignNodesToSkeletons()
    {
        // Every skeleton is a subtree.
        // A set of overlapping subtrees is a subtree as well.
        ForEach(trees_, [&](GLTFNode& child)
        {
            if (child.containedInSkins_.empty())
                return;

            const unsigned skeletonIndex = skinToSkeleton_[child.containedInSkins_[0]];
            for (unsigned i = 1; i < child.containedInSkins_.size(); ++i)
            {
                if (skeletonIndex != skinToSkeleton_[child.containedInSkins_[i]])
                    throw RuntimeException("Incorrect skeleton merge");
            }

            child.skeletonIndex_ = skeletonIndex;
        });
    }

    void InitializeSkeletonRootNode(GLTFSkeleton& skeleton) const
    {
        for (unsigned skinIndex : skeleton.skins_)
        {
            if (!skeleton.rootNode_)
                skeleton.rootNode_ = skinToRootNode_[skinIndex];
            else
                skeleton.rootNode_ = GetCommonParent(*skeleton.rootNode_, *skinToRootNode_[skinIndex]);

            if (!skeleton.rootNode_ || (skeleton.rootNode_->skeletonIndex_ != skeleton.index_))
                throw RuntimeException("Cannot find root of the skeleton when processing skin #{}", skinIndex);
        }
    }

    void AssignSkeletonBoneNames(GLTFSkeleton& skeleton) const
    {
        ForEachSkeletonNode(*skeleton.rootNode_, skeleton.index_,
            [&](GLTFNode& boneNode)
        {
            const ea::string nameHint = !boneNode.name_.empty() ? boneNode.name_ : "Bone";

            bool success = false;
            for (unsigned i = 0; i < MaxNameAssignTries; ++i)
            {
                const ea::string_view nameFormat = i != 0 ? "{0}_{1}" : "{0}";
                const ea::string name = Format(nameFormat, nameHint, i);
                if (skeleton.boneNameToNode_.contains(name))
                    continue;

                boneNode.uniqueBoneName_ = name;
                skeleton.boneNameToNode_.emplace(name, &boneNode);
                success = true;
                break;
            }

            if (!success)
                throw RuntimeException("Failed to assign name to bone");
        });
    }

    void InitializeSkins()
    {
        const unsigned numSkins = model_.skins.size();
        skins_.resize(numSkins);
        for (unsigned skinIndex = 0; skinIndex < numSkins; ++skinIndex)
        {
            GLTFSkin& skin = skins_[skinIndex];
            skin.index_ = skinIndex;
            InitializeSkin(skin);
        }
    }

    void InitializeSkin(GLTFSkin& skin) const
    {
        const tg::Skin& sourceSkin = model_.skins[skin.index_];
        const GLTFSkeleton& skeleton = skeletons_[skinToSkeleton_[skin.index_]];

        skin.skeleton_ = &skeleton;

        // Fill joints first
        ea::unordered_set<unsigned> jointNodes;
        for (int jointNodeIndex : sourceSkin.joints)
        {
            const GLTFNode& jointNode = GetNode(jointNodeIndex);
            if (!jointNode.uniqueBoneName_)
                throw RuntimeException("Cannot use node #{} in skin #{}", jointNodeIndex, skin.index_);

            skin.boneNodes_.push_back(&jointNode);
            jointNodes.insert(jointNodeIndex);
        }

        // Fill other nodes
        ForEachSkeletonNode(*skeleton.rootNode_, skeleton.index_,
            [&](GLTFNode& boneNode)
        {
            if (jointNodes.contains(boneNode.index_))
                return;
            if (!boneNode.uniqueBoneName_)
                throw RuntimeException("Cannot use node #{} in skin #{}", boneNode.index_, skin.index_);

            skin.boneNodes_.push_back(&boneNode);
        });

        // Fill bind matrices
        const unsigned numBones = skin.boneNodes_.size();
        skin.inverseBindMatrices_.resize(numBones);
        if (sourceSkin.inverseBindMatrices >= 0)
        {
            base_.CheckAccessor(sourceSkin.inverseBindMatrices);
            const tg::Accessor& accessor = model_.accessors[sourceSkin.inverseBindMatrices];
            const auto sourceBindMatrices = bufferReader_.ReadAccessorChecked<Matrix4>(accessor);

            if (sourceSkin.joints.size() > sourceBindMatrices.size())
                throw RuntimeException("Unexpected size of bind matrices array");

            for (unsigned i = 0; i < sourceSkin.joints.size(); ++i)
            {
                skin.inverseBindMatrices_[i] = Matrix3x4{ sourceBindMatrices[i].Transpose() };
                if (isDeepMirrored_)
                    skin.inverseBindMatrices_[i] = MirrorX(skin.inverseBindMatrices_[i]);
            }
        }

        // Generate skeleton bones
        skin.cookedBones_.resize(numBones);
        for (unsigned boneIndex = 0; boneIndex < numBones; ++boneIndex)
        {
            BoneView& bone = skin.cookedBones_[boneIndex];
            const GLTFNode& boneNode = *skin.boneNodes_[boneIndex];

            if (&boneNode != skeleton.rootNode_)
            {
                if (!boneNode.parent_)
                    throw RuntimeException("Bone parent must be present for child node");
                bone.parentIndex_ = skin.boneNodes_.index_of(boneNode.parent_);
                if (bone.parentIndex_ >= numBones)
                    throw RuntimeException("Bone parent must be within the skeleton");
            }

            bone.name_ = *boneNode.uniqueBoneName_;
            bone.SetInitialTransform(boneNode.position_, boneNode.rotation_, boneNode.scale_);
            if (boneIndex < skin.inverseBindMatrices_.size())
                bone.offsetMatrix_ = skin.inverseBindMatrices_[boneIndex];
            bone.SetLocalBoundingSphere(0.1f); // TODO: Remove this hack
        }
    }

    void AssignSkinnedModelsToNodes()
    {
        ForEach(trees_, [&](GLTFNode& node)
        {
            if (node.mesh_ && node.skin_)
            {
                const unsigned skeletonIndex = skinToSkeleton_[*node.skin_];
                GLTFSkeleton& skeleton = skeletons_[skeletonIndex];
                skeleton.rootNode_->skinnedMeshNodes_.emplace_back(node.index_);
            }
        });
    }

    void EnumerateUniqueMeshSkinPairs()
    {
        ForEach(trees_, [&](const GLTFNode& node)
        {
            if (node.mesh_ && node.skin_)
            {
                auto key = ea::make_pair<int, int>(*node.mesh_, *node.skin_);
                meshSkinPairs_[key] = GetOrCreateMatchingMeshSkinPair(*node.mesh_, node.skin_);
            }
        });

        ForEach(trees_, [&](const GLTFNode& node)
        {
            if (node.mesh_ && !node.skin_)
            {
                auto key = ea::make_pair<int, int>(*node.mesh_, -1);
                meshSkinPairs_[key] = GetOrCreateMatchingMeshSkinPair(*node.mesh_, ea::nullopt);
            }
        });
    }

    unsigned GetOrCreateMatchingMeshSkinPair(unsigned meshIndex, ea::optional<unsigned> skinIndex)
    {
        for (unsigned pairIndex = 0; pairIndex < uniqueMeshSkinPairs_.size(); ++pairIndex)
        {
            const GLTFMeshSkinPair& existingMeshSkin = *uniqueMeshSkinPairs_[pairIndex];
            if (!existingMeshSkin.skin_ && skinIndex)
                throw RuntimeException("Skinned meshes should be processed before non-skinned");

            // Always skip other meshes
            if (existingMeshSkin.mesh_ != meshIndex)
                continue;

            // Match non-skinned model to the first mesh
            if (!skinIndex || skinIndex == existingMeshSkin.skin_)
                return pairIndex;

            const GLTFSkin& existingSkin = skins_[*existingMeshSkin.skin_];
            const GLTFSkin& newSkin = skins_[*skinIndex];

            const bool areBonesMatching = ea::identical(
                existingSkin.cookedBones_.begin(), existingSkin.cookedBones_.end(),
                newSkin.cookedBones_.begin(), newSkin.cookedBones_.end(),
                [](const BoneView& lhs, const BoneView& rhs)
            {
                if (lhs.name_ != rhs.name_)
                    return false;
                if (lhs.parentIndex_ != rhs.parentIndex_)
                    return false;
                if (!lhs.offsetMatrix_.Equals(rhs.offsetMatrix_, 0.00002f)) // TODO: Make configurable
                    return false;
                // Don't compare initial transforms and bounding shapes
                return true;
            });

            if (areBonesMatching)
                return pairIndex;
        }

        const unsigned pairIndex = uniqueMeshSkinPairs_.size();
        auto pair = ea::make_shared<GLTFMeshSkinPair>(GLTFMeshSkinPair{ meshIndex, skinIndex });
        uniqueMeshSkinPairs_.push_back(pair);
        return pairIndex;
    }

private:
    static bool IsChildOf(const GLTFNode& child, const GLTFNode& parent)
    {
        return child.parent_ == &parent || (child.parent_ && IsChildOf(*child.parent_, parent));
    }

    static ea::vector<GLTFNode*> GetPathIncludingSelf(GLTFNode& node)
    {
        ea::vector<GLTFNode*> path;
        path.push_back(&node);

        GLTFNode* currentParent = node.parent_;
        while (currentParent)
        {
            path.push_back(currentParent);
            currentParent = currentParent->parent_;
        }

        ea::reverse(path.begin(), path.end());
        return path;
    }

    static GLTFNode* GetCommonParent(GLTFNode& lhs, GLTFNode& rhs)
    {
        if (lhs.root_ != rhs.root_)
            return nullptr;

        const auto lhsPath = GetPathIncludingSelf(lhs);
        const auto rhsPath = GetPathIncludingSelf(rhs);

        const unsigned numCommonParents = ea::min(lhsPath.size(), rhsPath.size());
        for (int i = numCommonParents - 1; i >= 0; --i)
        {
            if (lhsPath[i] == rhsPath[i])
                return lhsPath[i];
        }

        assert(0);
        return nullptr;
    }

    static void MarkInSkin(GLTFNode& node, unsigned skin)
    {
        if (!node.containedInSkins_.contains(skin))
            node.containedInSkins_.push_back(skin);
    }

    template <class T>
    static void ForEachInPathExceptParent(GLTFNode& child, GLTFNode& parent, const T& callback)
    {
        if (&child == &parent)
            return;
        if (!IsChildOf(child, parent))
            throw RuntimeException("Invalid ForEachInPath call");

        GLTFNode* node = &child;
        while (node != &parent)
        {
            callback(*node);
            node = node->parent_;
        }
    }

    template <class T>
    static void ForEachChild(GLTFNode& parent, const T& callback)
    {
        for (const GLTFNodePtr& child : parent.children_)
        {
            callback(*child);
            ForEachChild(*child, callback);
        }
    }

    template <class T>
    static void ForEach(ea::span<const GLTFNodePtr> nodes, const T& callback)
    {
        for (const GLTFNodePtr& node : nodes)
        {
            callback(*node);
            ForEachChild(*node, callback);
        }
    }

    template <class T>
    static void ForEachSkeletonNode(GLTFNode& skeletonRoot, unsigned skeletonIndex, const T& callback)
    {
        if (skeletonRoot.skeletonIndex_ != skeletonIndex)
            throw RuntimeException("Invalid call to ForEachSkeletonNode");

        callback(skeletonRoot);
        for (const GLTFNodePtr& child : skeletonRoot.children_)
        {
            if (child->skeletonIndex_ != skeletonIndex)
                continue;

            ForEachSkeletonNode(*child, skeletonIndex, callback);
        }
    }

    static Matrix3x4 ReadMatrix3x4(const std::vector<double>& src)
    {
        if (src.size() != 16)
            throw RuntimeException("Unexpected size of matrix object");

        Matrix4 temp;
        ea::transform(src.begin(), src.end(), &temp.m00_, StaticCaster<float>{});

        return Matrix3x4{ temp.Transpose() };
    }

    static Vector3 ReadVector3(const std::vector<double>& src)
    {
        if (src.size() != 3)
            throw RuntimeException("Unexpected size of matrix object");

        Vector3 temp;
        ea::transform(src.begin(), src.end(), &temp.x_, StaticCaster<float>{});
        return temp;
    }

    static Quaternion ReadQuaternion(const std::vector<double>& src)
    {
        if (src.size() != 4)
            throw RuntimeException("Unexpected size of matrix object");

        Vector4 temp;
        ea::transform(src.begin(), src.end(), &temp.x_, StaticCaster<float>{});
        return { temp.w_, temp.x_, temp.y_, temp.z_ };
    }

private:
    const GLTFImporterBase& base_;
    const GLTFBufferReader& bufferReader_;
    const tg::Model& model_;

    ea::vector<ea::optional<unsigned>> nodeToParent_;

    ea::vector<GLTFNodePtr> trees_;
    ea::vector<GLTFNode*> nodeToTreeNode_;
    bool isDeepMirrored_{};

    ea::vector<GLTFNode*> skinToRootNode_;
    ea::vector<unsigned> skinToSkeleton_;

    ea::vector<GLTFSkeleton> skeletons_;
    ea::vector<GLTFSkin> skins_;

    ea::unordered_map<ea::pair<int, int>, unsigned> meshSkinPairs_;
    ea::vector<GLTFMeshSkinPairPtr> uniqueMeshSkinPairs_;
};

/// Utility to import textures on-demand.
/// Textures cannot be imported after cooking.
class GLTFTextureImporter : public NonCopyable
{
public:
    explicit GLTFTextureImporter(GLTFImporterBase& base)
        : base_(base)
        , model_(base_.GetModel())
    {
        const unsigned numTextures = model_.textures.size();
        texturesAsIs_.resize(numTextures);
        for (unsigned i = 0; i < numTextures; ++i)
            texturesAsIs_[i] = ImportTexture(i, model_.textures[i]);
    }

    void CookTextures()
    {
        if (texturesCooked_)
            throw RuntimeException("Textures are already cooking");

        texturesCooked_ = true;
        for (auto& [indices, texture] : texturesMRO_)
        {
            const auto [metallicRoughnessTextureIndex, occlusionTextureIndex] = indices;

            texture.repackedImage_ = ImportRMOTexture(metallicRoughnessTextureIndex, occlusionTextureIndex,
                texture.fakeTexture_->GetName());
        }
    }

    void SaveResources()
    {
        for (const ImportedTexture& texture : texturesAsIs_)
        {
            if (!texture.isReferenced_)
                continue;
            base_.SaveResource(texture.image_);
            if (auto xmlFile = texture.cookedSamplerParams_)
                xmlFile->SaveFile(xmlFile->GetAbsoluteFileName());
        }

        for (const auto& elem : texturesMRO_)
        {
            const ImportedRMOTexture& texture = elem.second;
            base_.SaveResource(texture.repackedImage_);
            if (auto xmlFile = texture.cookedSamplerParams_)
                xmlFile->SaveFile(xmlFile->GetAbsoluteFileName());
        }
    }

    SharedPtr<Texture2D> ReferenceTextureAsIs(int textureIndex)
    {
        if (texturesCooked_)
            throw RuntimeException("Cannot reference textures after cooking");

        if (textureIndex >= texturesAsIs_.size())
            throw RuntimeException("Invalid texture #{} is referenced", textureIndex);

        ImportedTexture& texture = texturesAsIs_[textureIndex];
        texture.isReferenced_ = true;
        return texture.fakeTexture_;
    }

    SharedPtr<Texture2D> ReferenceRoughnessMetallicOcclusionTexture(
        int metallicRoughnessTextureIndex, int occlusionTextureIndex)
    {
        if (texturesCooked_)
            throw RuntimeException("Cannot reference textures after cooking");

        if (metallicRoughnessTextureIndex < 0 && occlusionTextureIndex < 0)
            throw RuntimeException("At least one texture should be referenced");
        if (metallicRoughnessTextureIndex >= 0 && metallicRoughnessTextureIndex >= texturesAsIs_.size())
            throw RuntimeException("Invalid metallic-roughness texture #{} is referenced", metallicRoughnessTextureIndex);
        if (occlusionTextureIndex >= 0 && occlusionTextureIndex >= texturesAsIs_.size())
            throw RuntimeException("Invalid occlusion texture #{} is referenced", occlusionTextureIndex);

        const auto key = ea::make_pair(metallicRoughnessTextureIndex, occlusionTextureIndex);
        const auto partialKeyA = ea::make_pair(metallicRoughnessTextureIndex, -1);
        const auto partialKeyB = ea::make_pair(-1, occlusionTextureIndex);

        // Try to find exact match
        auto iter = texturesMRO_.find(key);
        if (iter != texturesMRO_.end())
            return iter->second.fakeTexture_;

        // Try to re-purpose partial match A
        iter = texturesMRO_.find(partialKeyA);
        if (iter != texturesMRO_.end())
        {
            assert(occlusionTextureIndex != -1);
            const ImportedRMOTexture result = iter->second;
            texturesMRO_.erase(iter);
            texturesMRO_.emplace(key, result);
            return result.fakeTexture_;
        }

        // Try to re-purpose partial match B
        iter = texturesMRO_.find(partialKeyB);
        if (iter != texturesMRO_.end())
        {
            assert(metallicRoughnessTextureIndex != -1);
            const ImportedRMOTexture result = iter->second;
            texturesMRO_.erase(iter);
            texturesMRO_.emplace(key, result);
            return result.fakeTexture_;
        }

        // Create new texture
        const ImportedTexture& referenceTexture = metallicRoughnessTextureIndex >= 0
            ? texturesAsIs_[metallicRoughnessTextureIndex]
            : texturesAsIs_[occlusionTextureIndex];

        const ea::string imageName = base_.GetResourceName(
            referenceTexture.nameHint_, "Textures/", "Texture", ".png");

        ImportedRMOTexture& result = texturesMRO_[key];
        result.fakeTexture_ = MakeShared<Texture2D>(base_.GetContext());
        result.fakeTexture_->SetName(imageName);
        result.cookedSamplerParams_ = CookSamplerParams(result.fakeTexture_, referenceTexture.samplerParams_);
        return result.fakeTexture_;
    }

    static bool LoadImageData(tg::Image* image, const int imageIndex, std::string*, std::string*,
        int reqWidth, int reqHeight, const unsigned char* bytes, int size, void*)
    {
        image->name = GetFileName(image->uri.c_str()).c_str();
        image->as_is = true;
        image->image.resize(size);
        ea::copy_n(bytes, size, image->image.begin());
        return true;
    }

private:
    struct SamplerParams
    {
        TextureFilterMode filterMode_{ FILTER_DEFAULT };
        bool mipmaps_{ true };
        TextureAddressMode wrapU_{ ADDRESS_WRAP };
        TextureAddressMode wrapV_{ ADDRESS_WRAP };
    };

    struct ImportedTexture
    {
        bool isReferenced_{};

        ea::string nameHint_;
        SharedPtr<BinaryFile> image_;
        SharedPtr<Texture2D> fakeTexture_;
        SamplerParams samplerParams_;
        SharedPtr<XMLFile> cookedSamplerParams_;
    };

    struct ImportedRMOTexture
    {
        SharedPtr<Texture2D> fakeTexture_;
        SharedPtr<XMLFile> cookedSamplerParams_;

        SharedPtr<Image> repackedImage_;
    };

    static TextureFilterMode GetFilterMode(const tg::Sampler& sampler)
    {
        if (sampler.minFilter == -1 || sampler.magFilter == -1)
            return FILTER_DEFAULT;
        else if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
        {
            if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
                || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST)
                return FILTER_NEAREST;
            else
                return FILTER_NEAREST_ANISOTROPIC;
        }
        else
        {
            if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST)
                return FILTER_BILINEAR;
            else
                return FILTER_DEFAULT;
        }
    }

    static bool HasMipmaps(const tg::Sampler& sampler)
    {
        return sampler.minFilter == -1 || sampler.magFilter == -1
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    }

    static TextureAddressMode GetAddressMode(int sourceMode)
    {
        switch (sourceMode)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return ADDRESS_CLAMP;

        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return ADDRESS_MIRROR;

        case TINYGLTF_TEXTURE_WRAP_REPEAT:
        default:
            return ADDRESS_WRAP;
        }
    }

    SharedPtr<BinaryFile> ImportImageAsIs(unsigned imageIndex, const tg::Image& sourceImage) const
    {
        auto image = MakeShared<BinaryFile>(base_.GetContext());
        const ea::string imageUri = sourceImage.uri.c_str();

        if (sourceImage.mimeType == "image/jpeg" || imageUri.ends_with(".jpg") || imageUri.ends_with(".jpeg"))
        {
            const ea::string imageName = base_.GetResourceName(
                sourceImage.name.c_str(), "Textures/", "Texture", ".jpg");
            image->SetName(imageName);
        }
        else if (sourceImage.mimeType == "image/png" || imageUri.ends_with(".png"))
        {
            const ea::string imageName = base_.GetResourceName(
                sourceImage.name.c_str(), "Textures/", "Texture", ".png");
            image->SetName(imageName);
        }
        else
        {
            throw RuntimeException("Image #{} '{}' has unknown type '{}'",
                imageIndex, sourceImage.name.c_str(), sourceImage.mimeType.c_str());
        }

        ByteVector imageBytes;
        imageBytes.resize(sourceImage.image.size());
        ea::copy(sourceImage.image.begin(), sourceImage.image.end(), imageBytes.begin());
        image->SetData(imageBytes);
        return image;
    }

    SharedPtr<Image> DecodeImage(BinaryFile* imageAsIs) const
    {
        Deserializer& deserializer = imageAsIs->AsDeserializer();
        deserializer.Seek(0);

        auto decodedImage = MakeShared<Image>(base_.GetContext());
        decodedImage->SetName(imageAsIs->GetName());
        decodedImage->Load(deserializer);
        return decodedImage;
    }

    ImportedTexture ImportTexture(unsigned textureIndex, const tg::Texture& sourceTexture) const
    {
        base_.CheckImage(sourceTexture.source);

        const tg::Image& sourceImage = model_.images[sourceTexture.source];

        ImportedTexture result;
        result.nameHint_ = sourceImage.name.c_str();
        result.image_ = ImportImageAsIs(sourceTexture.source, sourceImage);
        result.fakeTexture_ = MakeShared<Texture2D>(base_.GetContext());
        result.fakeTexture_->SetName(result.image_->GetName());
        if (sourceTexture.sampler >= 0)
        {
            base_.CheckSampler(sourceTexture.sampler);

            const tg::Sampler& sourceSampler = model_.samplers[sourceTexture.sampler];
            result.samplerParams_.filterMode_ = GetFilterMode(sourceSampler);
            result.samplerParams_.mipmaps_ = HasMipmaps(sourceSampler);
            result.samplerParams_.wrapU_ = GetAddressMode(sourceSampler.wrapS);
            result.samplerParams_.wrapV_ = GetAddressMode(sourceSampler.wrapT);
        }
        result.cookedSamplerParams_ = CookSamplerParams(result.image_, result.samplerParams_);
        return result;
    }

    SharedPtr<XMLFile> CookSamplerParams(Resource* image, const SamplerParams& samplerParams) const
    {
        static const ea::string addressModeNames[] =
        {
            "wrap",
            "mirror",
            "",
            "border"
        };

        static const ea::string filterModeNames[] =
        {
            "nearest",
            "bilinear",
            "trilinear",
            "anisotropic",
            "nearestanisotropic",
            "default"
        };

        auto xmlFile = MakeShared<XMLFile>(base_.GetContext());

        XMLElement rootElement = xmlFile->CreateRoot("texture");

        if (samplerParams.wrapU_ != ADDRESS_WRAP)
        {
            XMLElement childElement = rootElement.CreateChild("address");
            childElement.SetAttribute("coord", "u");
            childElement.SetAttribute("mode", addressModeNames[samplerParams.wrapU_]);
        };

        if (samplerParams.wrapV_ != ADDRESS_WRAP)
        {
            XMLElement childElement = rootElement.CreateChild("address");
            childElement.SetAttribute("coord", "v");
            childElement.SetAttribute("mode", addressModeNames[samplerParams.wrapV_]);
        };

        if (samplerParams.filterMode_ != FILTER_DEFAULT)
        {
            XMLElement childElement = rootElement.CreateChild("filter");
            childElement.SetAttribute("mode", filterModeNames[samplerParams.filterMode_]);
        }

        if (!samplerParams.mipmaps_)
        {
            XMLElement childElement = rootElement.CreateChild("mipmap");
            childElement.SetBool("enable", false);
        }

        // Don't create XML if all parameters are default
        if (!rootElement.GetChild())
            return nullptr;

        const ea::string& imageName = image->GetName();
        xmlFile->SetName(ReplaceExtension(imageName, ".xml"));
        xmlFile->SetAbsoluteFileName(ReplaceExtension(base_.GetAbsoluteFileName(imageName), ".xml"));
        return xmlFile;
    }

    SharedPtr<Image> ImportRMOTexture(
        int metallicRoughnessTextureIndex, int occlusionTextureIndex, const ea::string& name)
    {
        // Unpack input images
        SharedPtr<Image> metallicRoughnessImage = metallicRoughnessTextureIndex >= 0
            ? DecodeImage(texturesAsIs_[metallicRoughnessTextureIndex].image_)
            : nullptr;

        SharedPtr<Image> occlusionImage = occlusionTextureIndex >= 0
            ? DecodeImage(texturesAsIs_[occlusionTextureIndex].image_)
            : nullptr;

        if (!metallicRoughnessImage && !occlusionImage)
        {
            throw RuntimeException("Neither metallic-roughness texture #{} nor occlusion texture #{} can be loaded",
                metallicRoughnessTextureIndex, occlusionTextureIndex);
        }

        const IntVector3 metallicRoughnessImageSize = metallicRoughnessImage ? metallicRoughnessImage->GetSize() : IntVector3::ZERO;
        const IntVector3 occlusionImageSize = occlusionImage ? occlusionImage->GetSize() : IntVector3::ZERO;
        const IntVector2 repackedImageSize = VectorMax(metallicRoughnessImageSize.ToVector2(), occlusionImageSize.ToVector2());

        if (repackedImageSize.x_ <= 0 || repackedImageSize.y_ <= 0)
            throw RuntimeException("Repacked metallic-roughness-occlusion texture has invalid size");

        if (metallicRoughnessImage && metallicRoughnessImageSize.ToVector2() != repackedImageSize)
            metallicRoughnessImage->Resize(repackedImageSize.x_, repackedImageSize.y_);

        if (occlusionImage && occlusionImageSize.ToVector2() != repackedImageSize)
            occlusionImage->Resize(repackedImageSize.x_, repackedImageSize.y_);

        auto finalImage = MakeShared<Image>(base_.GetContext());
        finalImage->SetName(name);
        finalImage->SetSize(repackedImageSize.x_, repackedImageSize.y_, 1, occlusionImage ? 4 : 3);

        for (const IntVector2 texel : IntRect{ IntVector2::ZERO, repackedImageSize })
        {
            // 0xOO__MMRR
            unsigned color{};
            if (metallicRoughnessImage)
            {
                // 0x__MMRR__
                const unsigned value = metallicRoughnessImage->GetPixelInt(texel.x_, texel.y_);
                color |= (value >> 8) & 0xffff;
            }
            if (occlusionImage)
            {
                // 0x______OO
                const unsigned value = occlusionImage->GetPixelInt(texel.x_, texel.y_);
                color |= (value & 0xff) << 24;
            }
            else
            {
                color |= 0xff000000;
            }
            finalImage->SetPixelInt(texel.x_, texel.y_, color);
        }

        return finalImage;
    }

    GLTFImporterBase& base_;
    const tg::Model& model_;

    ea::vector<ImportedTexture> texturesAsIs_;
    ea::unordered_map<ea::pair<int, int>, ImportedRMOTexture> texturesMRO_;

    bool texturesCooked_{};
};

/// Utility to import materials.
class GLTFMaterialImporter : public NonCopyable
{
public:
    explicit GLTFMaterialImporter(GLTFImporterBase& base, GLTFTextureImporter& textureImporter)
        : base_(base)
        , model_(base_.GetModel())
        , textureImporter_(textureImporter)
    {
        for (const tg::Material& sourceMaterial : model_.materials)
            materials_.push_back(ImportMaterial(sourceMaterial));
        textureImporter_.CookTextures();
    }

    SharedPtr<Material> GetMaterial(int materialIndex) const
    {
        base_.CheckMaterial(materialIndex);
        return materials_[materialIndex];
    }

    void SaveResources()
    {
        for (const auto& material : materials_)
            base_.SaveResource(material);
    }

private:
    SharedPtr<Material> ImportMaterial(const tg::Material& sourceMaterial)
    {
        auto cache = base_.GetContext()->GetSubsystem<ResourceCache>();

        auto material = MakeShared<Material>(base_.GetContext());

        const tg::PbrMetallicRoughness& pbr = sourceMaterial.pbrMetallicRoughness;
        const Vector4 baseColor{ ToArray<float, 4>(pbr.baseColorFactor).data() };
        material->SetShaderParameter(ShaderConsts::Material_MatDiffColor, baseColor);
        material->SetShaderParameter(ShaderConsts::Material_Metallic, static_cast<float>(pbr.metallicFactor));
        material->SetShaderParameter(ShaderConsts::Material_Roughness, static_cast<float>(pbr.roughnessFactor));

        const ea::string techniqueName = "Techniques/LitOpaque.xml";
        auto technique = cache->GetResource<Technique>(techniqueName);
        if (!technique)
        {
            throw RuntimeException("Cannot find standard technique '{}' for material '{}'",
                techniqueName, sourceMaterial.name.c_str());
        }

        material->SetTechnique(0, technique);
        material->SetVertexShaderDefines("PBR");
        material->SetPixelShaderDefines("PBR");

        if (pbr.baseColorTexture.index >= 0)
        {
            if (pbr.baseColorTexture.texCoord != 0)
            {
                URHO3D_LOGWARNING("Material '{}' has non-standard UV for diffuse texture #{}",
                    sourceMaterial.name.c_str(), pbr.baseColorTexture.index);
            }

            const SharedPtr<Texture2D> diffuseTexture = textureImporter_.ReferenceTextureAsIs(
                pbr.baseColorTexture.index);
            material->SetTexture(TU_DIFFUSE, diffuseTexture);
        }

        // Occlusion and metallic-roughness textures are backed together,
        // ignore occlusion if is uses different UV.
        int occlusionTextureIndex = sourceMaterial.occlusionTexture.index;
        int metallicRoughnessTextureIndex = pbr.metallicRoughnessTexture.index;
        if (occlusionTextureIndex >= 0 && metallicRoughnessTextureIndex >= 0
            && sourceMaterial.occlusionTexture.texCoord != pbr.metallicRoughnessTexture.texCoord)
        {
            URHO3D_LOGWARNING("Material '{}' uses different UV for metallic-roughness texture #{} "
                "and for occlusion texture #{}. Occlusion texture is ignored.",
                sourceMaterial.name.c_str(), metallicRoughnessTextureIndex, occlusionTextureIndex);
            occlusionTextureIndex = -1;
        }

        if (metallicRoughnessTextureIndex >= 0 || occlusionTextureIndex >= 0)
        {
            if (metallicRoughnessTextureIndex >= 0 && pbr.metallicRoughnessTexture.texCoord != 0)
            {
                URHO3D_LOGWARNING("Material '{}' has non-standard UV for metallic-roughness texture #{}",
                    sourceMaterial.name.c_str(), metallicRoughnessTextureIndex);
            }

            if (occlusionTextureIndex >= 0)
            {
                if (sourceMaterial.occlusionTexture.texCoord != 0)
                {
                    URHO3D_LOGWARNING("Material '{}' has non-standard UV for occlusion texture #{}",
                        sourceMaterial.name.c_str(), occlusionTextureIndex);
                }
                if (sourceMaterial.occlusionTexture.strength != 1.0)
                {
                    URHO3D_LOGWARNING("Material '{}' has non-default occlusion strength for occlusion texture #{}",
                        sourceMaterial.name.c_str(), occlusionTextureIndex);
                }
            }

            const SharedPtr<Texture2D> metallicRoughnessTexture = textureImporter_.ReferenceRoughnessMetallicOcclusionTexture(
                metallicRoughnessTextureIndex, occlusionTextureIndex);
            material->SetTexture(TU_SPECULAR, metallicRoughnessTexture);
        }

        const ea::string materialName = base_.GetResourceName(
            sourceMaterial.name.c_str(), "Materials/", "Material", ".xml");
        material->SetName(materialName);

        base_.AddToResourceCache(material);
        return material;
    }

    GLTFImporterBase& base_;
    const tg::Model& model_;
    GLTFTextureImporter& textureImporter_;

    ea::vector<SharedPtr<Material>> materials_;
};

/// Utility to import models.
class GLTFModelImporter : public NonCopyable
{
public:
    explicit GLTFModelImporter(GLTFImporterBase& base,
        const GLTFBufferReader& bufferReader, const GLTFHierarchyAnalyzer& hierarchyAnalyzer,
        const GLTFMaterialImporter& materialImporter)
        : base_(base)
        , model_(base_.GetModel())
        , bufferReader_(bufferReader)
        , hierarchyAnalyzer_(hierarchyAnalyzer)
        , materialImporter_(materialImporter)
    {
        InitializeModels();
    }

    void SaveResources()
    {
        for (const ImportedModel& model : models_)
            base_.SaveResource(model.model_);
    }

    SharedPtr<Model> GetModel(int meshIndex, int skinIndex) const
    {
        return GetImportedModel(meshIndex, skinIndex).model_;
    }

    const StringVector& GetModelMaterials(int meshIndex, int skinIndex) const
    {
        return GetImportedModel(meshIndex, skinIndex).materials_;
    }

private:
    struct ImportedModel
    {
        GLTFNodePtr skeleton_;
        SharedPtr<ModelView> modelView_;
        SharedPtr<Model> model_;
        StringVector materials_;
    };
    using ImportedModelPtr = ea::shared_ptr<ImportedModel>;

    void InitializeModels()
    {
        for (const GLTFMeshSkinPairPtr& pair : hierarchyAnalyzer_.GetUniqueMeshSkinPairs())
        {
            const tg::Mesh& sourceMesh = model_.meshes[pair->mesh_];

            ImportedModel model;
            model.modelView_ = ImportModelView(sourceMesh, hierarchyAnalyzer_.GetSkinBones(pair->skin_));
            model.model_ = model.modelView_->ExportModel();
            model.materials_ = model.modelView_->ExportMaterialList();
            base_.AddToResourceCache(model.model_);
            models_.push_back(model);
        }
    }

    const ImportedModel& GetImportedModel(int meshIndex, int skinIndex) const
    {
        const unsigned modelIndex = hierarchyAnalyzer_.GetUniqueMeshSkin(meshIndex, skinIndex);
        return models_[modelIndex];
    }

    SharedPtr<ModelView> ImportModelView(const tg::Mesh& sourceMesh, const ea::vector<BoneView>& bones)
    {
        const ea::string modelName = base_.GetResourceName(sourceMesh.name.c_str(), "", "Model", ".mdl");

        auto modelView = MakeShared<ModelView>(base_.GetContext());
        modelView->SetName(modelName);
        modelView->SetBones(bones);

        const unsigned numMorphWeights = sourceMesh.weights.size();
        for (unsigned morphIndex = 0; morphIndex < numMorphWeights; ++morphIndex)
            modelView->SetMorph(morphIndex, { "", static_cast<float>(sourceMesh.weights[morphIndex]) });

        auto& geometries = modelView->GetGeometries();

        const unsigned numGeometries = sourceMesh.primitives.size();
        geometries.resize(numGeometries);
        for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
        {
            GeometryView& geometryView = geometries[geometryIndex];
            geometryView.lods_.resize(1);
            GeometryLODView& geometryLODView = geometryView.lods_[0];

            const tg::Primitive& primitive = sourceMesh.primitives[geometryIndex];
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                URHO3D_LOGWARNING("Unsupported geometry type {} in mesh '{}'.", primitive.mode, sourceMesh.name.c_str());
                return nullptr;
            }

            if (primitive.attributes.empty())
            {
                URHO3D_LOGWARNING("No attributes in primitive #{} in mesh '{}'.", geometryIndex, sourceMesh.name.c_str());
                return nullptr;
            }

            if (primitive.indices >= 0)
            {
                base_.CheckAccessor(primitive.indices);
                geometryLODView.indices_ = bufferReader_.ReadAccessorChecked<unsigned>(model_.accessors[primitive.indices]);
            }

            const unsigned numVertices = model_.accessors[primitive.attributes.begin()->second].count;
            geometryLODView.vertices_.resize(numVertices);
            for (const auto& attribute : primitive.attributes)
            {
                const tg::Accessor& accessor = model_.accessors[attribute.second];
                if (!ReadVertexData(geometryLODView.vertexFormat_, geometryLODView.vertices_,
                    attribute.first.c_str(), accessor))
                {
                    URHO3D_LOGWARNING("Cannot read primitive #{} in mesh '{}'.", geometryIndex, sourceMesh.name.c_str());
                    return nullptr;
                }
            }

            if (primitive.material >= 0)
            {
                if (auto material = materialImporter_.GetMaterial(primitive.material))
                    geometryView.material_ = material->GetName();
            }

            if (numMorphWeights > 0 && primitive.targets.size() != numMorphWeights)
            {
                throw RuntimeException("Primitive #{} in mesh '{}' has incorrect number of morph weights.",
                    geometryIndex, sourceMesh.name.c_str());
            }

            for (unsigned morphIndex = 0; morphIndex < primitive.targets.size(); ++morphIndex)
            {
                const auto& morphAttributes = primitive.targets[morphIndex];
                geometryLODView.morphs_[morphIndex] = ReadVertexMorphs(morphAttributes, numVertices);
            }
        }

        if (hierarchyAnalyzer_.IsDeepMirrored())
            modelView->MirrorGeometriesX();

        modelView->CalculateMissingNormalsSmooth(); // TODO: Should be flat
        modelView->Normalize();
        return modelView;
    }

    bool ReadVertexData(ModelVertexFormat& vertexFormat, ea::vector<ModelVertex>& vertices,
        const ea::string& semantics, const tg::Accessor& accessor)
    {
        const auto& parsedSemantics = semantics.split('_');
        const ea::string& semanticsName = parsedSemantics[0];
        const unsigned semanticsIndex = parsedSemantics.size() > 1 ? FromString<unsigned>(parsedSemantics[1]) : 0;

        if (semanticsName == "POSITION" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3)
            {
                URHO3D_LOGERROR("Unexpected type of vertex position");
                return false;
            }

            vertexFormat.position_ = TYPE_VECTOR3;

            const auto positions = bufferReader_.ReadAccessorChecked<Vector3>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].SetPosition(positions[i]);
        }
        else if (semanticsName == "NORMAL" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3)
            {
                URHO3D_LOGERROR("Unexpected type of vertex normal");
                return false;
            }

            vertexFormat.normal_ = TYPE_VECTOR3;

            const auto normals = bufferReader_.ReadAccessorChecked<Vector3>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].SetNormal(normals[i].Normalized());
        }
        else if (semanticsName == "TANGENT" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC4)
            {
                URHO3D_LOGERROR("Unexpected type of vertex tangent");
                return false;
            }

            vertexFormat.tangent_ = TYPE_VECTOR4;

            const auto tangents = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].tangent_ = tangents[i];
        }
        else if (semanticsName == "TEXCOORD" && semanticsIndex < ModelVertex::MaxUVs)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC2)
            {
                URHO3D_LOGERROR("Unexpected type of vertex uv");
                return false;
            }

            vertexFormat.uv_[semanticsIndex] = TYPE_VECTOR2;

            const auto uvs = bufferReader_.ReadAccessorChecked<Vector2>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].uv_[semanticsIndex] = { uvs[i], Vector2::ZERO };
        }
        else if (semanticsName == "COLOR" && semanticsIndex < ModelVertex::MaxColors)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3 && accessor.type != TINYGLTF_TYPE_VEC4)
            {
                URHO3D_LOGERROR("Unexpected type of vertex color");
                return false;
            }

            if (accessor.type == TINYGLTF_TYPE_VEC3)
            {
                vertexFormat.color_[semanticsIndex] = TYPE_VECTOR3;

                const auto colors = bufferReader_.ReadAccessorChecked<Vector3>(accessor);
                for (unsigned i = 0; i < accessor.count; ++i)
                    vertices[i].color_[semanticsIndex] = { colors[i], 1.0f };
            }
            else if (accessor.type == TINYGLTF_TYPE_VEC4)
            {
                vertexFormat.color_[semanticsIndex] = TYPE_VECTOR4;

                const auto colors = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
                for (unsigned i = 0; i < accessor.count; ++i)
                    vertices[i].color_[semanticsIndex] = colors[i];
            }
        }
        else if (semanticsName == "JOINTS" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC4)
                throw RuntimeException("Unexpected type of skin joints");

            vertexFormat.blendIndices_ = TYPE_UBYTE4;

            const auto indices = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].blendIndices_ = indices[i];
        }
        else if (semanticsName == "WEIGHTS" && semanticsIndex == 0)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC4)
                throw RuntimeException("Unexpected type of skin weights");

            vertexFormat.blendWeights_ = TYPE_UBYTE4_NORM;

            const auto weights = bufferReader_.ReadAccessorChecked<Vector4>(accessor);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].blendWeights_ = weights[i];
        }

        return true;
    }

    ModelVertexMorphVector ReadVertexMorphs(const std::map<std::string, int>& accessors, unsigned numVertices)
    {
        ea::vector<Vector3> positionDeltas(numVertices);
        ea::vector<Vector3> normalDeltas(numVertices);
        ea::vector<Vector3> tangentDeltas(numVertices);

        if (const auto positionIter = accessors.find("POSITION"); positionIter != accessors.end())
        {
            base_.CheckAccessor(positionIter->second);
            positionDeltas = bufferReader_.ReadAccessor<Vector3>(model_.accessors[positionIter->second]);
        }

        if (const auto normalIter = accessors.find("NORMAL"); normalIter != accessors.end())
        {
            base_.CheckAccessor(normalIter->second);
            normalDeltas = bufferReader_.ReadAccessor<Vector3>(model_.accessors[normalIter->second]);
        }

        if (const auto tangentIter = accessors.find("TANGENT"); tangentIter != accessors.end())
        {
            base_.CheckAccessor(tangentIter->second);
            tangentDeltas = bufferReader_.ReadAccessor<Vector3>(model_.accessors[tangentIter->second]);
        }

        if (numVertices != positionDeltas.size() || numVertices != normalDeltas.size() || numVertices != tangentDeltas.size())
            throw RuntimeException("Morph target has inconsistent sizes of accessors");

        ModelVertexMorphVector vertexMorphs(numVertices);
        for (unsigned i = 0; i < numVertices; ++i)
        {
            vertexMorphs[i].index_ = i;
            vertexMorphs[i].positionDelta_ = positionDeltas[i];
            vertexMorphs[i].normalDelta_ = normalDeltas[i];
            vertexMorphs[i].tangentDelta_ = tangentDeltas[i];
        }
        return vertexMorphs;
    }

    GLTFImporterBase& base_;
    const tg::Model& model_;
    const GLTFBufferReader& bufferReader_;
    const GLTFHierarchyAnalyzer& hierarchyAnalyzer_;
    const GLTFMaterialImporter& materialImporter_;

    ea::vector<ImportedModel> models_;
};

tg::Model LoadGLTF(const ea::string& fileName)
{
    tg::TinyGLTF loader;
    loader.SetImageLoader(&GLTFTextureImporter::LoadImageData, nullptr);

    std::string errorMessage;
    tg::Model model;
    if (!loader.LoadASCIIFromFile(&model, &errorMessage, nullptr, fileName.c_str()))
        throw RuntimeException("Failed to import GLTF file: {}", errorMessage.c_str());

    return model;
}

}

class GLTFImporter::Impl
{
public:
    explicit Impl(Context* context, const ea::string& fileName,
        const ea::string& outputPath, const ea::string& resourceNamePrefix)
        : context_(context)
        , importerContext_(context, LoadGLTF(fileName), outputPath, resourceNamePrefix)
        , bufferReader_(importerContext_)
        , hierarchyAnalyzer_(importerContext_, bufferReader_)
        , textureImporter_(importerContext_)
        , materialImporter_(importerContext_, textureImporter_)
        , modelImporter_(importerContext_, bufferReader_, hierarchyAnalyzer_, materialImporter_)
    {
        // TODO: Remove me
        model_ = importerContext_.GetModel();
    }

    bool CookResources()
    {
        for (const tg::Scene& sourceScene : model_.scenes)
        {
            const auto scene = ImportScene(sourceScene);
            importedScenes_.push_back(scene);
        }

        return true;
    }

    bool SaveResources()
    {
        textureImporter_.SaveResources();
        materialImporter_.SaveResources();
        modelImporter_.SaveResources();

        for (Scene* scene : importedScenes_)
            importerContext_.SaveResource(scene);

        return true;
    }

private:
    SharedPtr<Scene> ImportScene(const tg::Scene& sourceScene)
    {
        nodeToIndex_.clear();
        indexToNode_.clear();

        auto cache = context_->GetSubsystem<ResourceCache>();
        const ea::string sceneName = importerContext_.GetResourceName(sourceScene.name.c_str(), "", "Scene", ".xml");

        auto scene = MakeShared<Scene>(context_);
        scene->SetFileName(importerContext_.GetAbsoluteFileName(sceneName));
        scene->CreateComponent<Octree>();

        for (int nodeIndex : sourceScene.nodes)
        {
            ImportNode(scene, hierarchyAnalyzer_.GetNode(nodeIndex));
        }

        static const Vector3 defaultPosition{ -1.0f, 2.0f, 1.0f };

        if (!scene->GetComponent<Light>(true))
        {
            // Model forward is Z+, make default lighting from top right when looking at forward side of model.
            Node* node = scene->CreateChild("Default Light");
            node->SetPosition(defaultPosition);
            node->SetDirection({ 1.0f, -2.0f, -1.0f });
            auto light = node->CreateComponent<Light>();
            light->SetLightType(LIGHT_DIRECTIONAL);
        }

        if (!scene->GetComponent<Zone>(true) && !scene->GetComponent<Skybox>(true))
        {
            auto skyboxMaterial = cache->GetResource<Material>("Materials/Skybox.xml");
            auto skyboxTexture = cache->GetResource<TextureCube>("Textures/Skybox.xml");
            auto boxModel = cache->GetResource<Model>("Models/Box.mdl");

            if (skyboxMaterial && skyboxTexture && boxModel)
            {
                Node* zoneNode = scene->CreateChild("Default Zone");
                zoneNode->SetPosition(defaultPosition);
                auto zone = zoneNode->CreateComponent<Zone>();
                zone->SetBackgroundBrightness(0.5f);
                zone->SetZoneTexture(skyboxTexture);

                Node* skyboxNode = scene->CreateChild("Default Skybox");
                skyboxNode->SetPosition(defaultPosition);
                auto skybox = skyboxNode->CreateComponent<Skybox>();
                skybox->SetModel(boxModel);
                skybox->SetMaterial(skyboxMaterial);
            }

        }

        return scene;
    }

    void RegisterNode(Node& node, const GLTFNode& sourceNode)
    {
        indexToNode_[sourceNode.index_] = &node;
        nodeToIndex_[&node] = sourceNode.index_;
    }

    void ImportNode(Node* parent, const GLTFNode& sourceNode)
    {
        auto cache = context_->GetSubsystem<ResourceCache>();

        // Skip skinned mesh nodes w/o children because Urho instantiates such nodes at skeleton root.
        if (sourceNode.mesh_ && sourceNode.skin_ && sourceNode.children_.empty() && sourceNode.skinnedMeshNodes_.empty())
            return;

        Node* node = nullptr;
        if (!sourceNode.skeletonIndex_ || !sourceNode.skinnedMeshNodes_.empty())
            node = parent->CreateChild(hierarchyAnalyzer_.GetEffectiveNodeName(sourceNode));
        else
        {
            node = indexToNode_[sourceNode.index_];
            if (!node)
                throw RuntimeException("Cannot find bone node #{}", sourceNode.index_);
        }

        RegisterNode(*node, sourceNode);

        if (!sourceNode.skinnedMeshNodes_.empty())
        {
            for (const unsigned nodeIndex : sourceNode.skinnedMeshNodes_)
            {
                const GLTFNode& meshNode = hierarchyAnalyzer_.GetNode(nodeIndex);
                Model* model = modelImporter_.GetModel(*meshNode.mesh_, *meshNode.skin_);
                if (!model)
                    continue;

                auto animatedModel = node->CreateComponent<AnimatedModel>();
                animatedModel->SetModel(model);

                const StringVector& meshMaterials = modelImporter_.GetModelMaterials(*meshNode.mesh_, *meshNode.skin_);
                for (unsigned i = 0; i < meshMaterials.size(); ++i)
                {
                    auto material = cache->GetResource<Material>(meshMaterials[i]);
                    animatedModel->SetMaterial(i, material);
                }
            }

            if (node->GetNumChildren() != 1)
                throw RuntimeException("Cannot connect node #{} to its children", sourceNode.index_);

            // Connect bone nodes to GLTF nodes
            Node* skeletonRootNode = node->GetChild(0u);
            skeletonRootNode->SetTransform(sourceNode.position_, sourceNode.rotation_, sourceNode.scale_);

            const GLTFSkeleton& skeleton = hierarchyAnalyzer_.GetSkeleton(*sourceNode.skeletonIndex_);
            for (const auto& [boneName, boneSourceNode] : skeleton.boneNameToNode_)
            {
                Node* boneNode = skeletonRootNode->GetName() == boneName ? skeletonRootNode : skeletonRootNode->GetChild(boneName, true);
                if (!boneNode)
                    throw RuntimeException("Cannot connect node #{} to skeleton bone", boneSourceNode->index_, boneName);

                RegisterNode(*boneNode, *boneSourceNode);
            }

            for (const GLTFNodePtr& childNode : sourceNode.children_)
            {
                ImportNode(node->GetChild(0u), *childNode);
            }
        }
        else
        {
            // Skip skinned mesh nodes because Urho instantiates such nodes at skeleton root.
            if (sourceNode.mesh_ && sourceNode.skin_ && sourceNode.children_.empty())
                return;

            node->SetTransform(sourceNode.position_, sourceNode.rotation_, sourceNode.scale_);

            if (sourceNode.mesh_ && !sourceNode.skin_)
            {
                if (Model* model = modelImporter_.GetModel(*sourceNode.mesh_, -1))
                {
                    const bool needAnimation = model->GetNumMorphs() > 0;// || model->GetSkeleton().GetNumBones() > 1;
                    auto staticModel = !needAnimation
                        ? node->CreateComponent<StaticModel>()
                        : node->CreateComponent<AnimatedModel>();

                    staticModel->SetModel(model);

                    const StringVector& meshMaterials = modelImporter_.GetModelMaterials(*sourceNode.mesh_, -1);
                    for (unsigned i = 0; i < meshMaterials.size(); ++i)
                    {
                        auto material = cache->GetResource<Material>(meshMaterials[i]);
                        staticModel->SetMaterial(i, material);
                    }
                }
            }

            for (const GLTFNodePtr& childNode : sourceNode.children_)
            {
                ImportNode(node, *childNode);
            }
        }
    }

    GLTFImporterBase importerContext_;
    const GLTFBufferReader bufferReader_;
    const GLTFHierarchyAnalyzer hierarchyAnalyzer_;
    GLTFTextureImporter textureImporter_;
    GLTFMaterialImporter materialImporter_;
    GLTFModelImporter modelImporter_;

    Context* context_{};

    /// Initialized after loading
    /// @{
    tg::Model model_;
    /// @}

    /// Initialized after cooking
    /// @{
    ea::vector<SharedPtr<Scene>> importedScenes_;
    ea::unordered_map<Node*, unsigned> nodeToIndex_;
    ea::unordered_map<unsigned, Node*> indexToNode_;
    /// @}
};

GLTFImporter::GLTFImporter(Context* context)
    : Object(context)
{

}

GLTFImporter::~GLTFImporter()
{

}

bool GLTFImporter::LoadFile(const ea::string& fileName,
    const ea::string& outputPath, const ea::string& resourceNamePrefix)
{
    try
    {
        impl_ = ea::make_unique<Impl>(context_, fileName, outputPath, resourceNamePrefix);
        return true;
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

bool GLTFImporter::CookResources()
{
    try
    {
        if (!impl_)
            throw RuntimeException("GLTF file wasn't loaded");

        return impl_->CookResources();
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

bool GLTFImporter::SaveResources()
{
    try
    {
        if (!impl_)
            throw RuntimeException("Imported asserts weren't cooked");

        return impl_->SaveResources();
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

}
