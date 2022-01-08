//
// Copyright (c) 2017-2021 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"
#include "../NetworkUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Network/DefaultNetworkObject.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Network/NetworkManager.h>
#include <Urho3D/Network/NetworkObject.h>
#include <Urho3D/Network/NetworkValue.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Resource/XMLFile.h>

namespace
{

SharedPtr<XMLFile> CreateTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);
    node->SetName("Root");
    auto staticModel = node->CreateComponent<StaticModel>();
    staticModel->SetCastShadows(true);

    auto childNode = node->CreateChild("Child");
    childNode->SetPosition({0.0f, 1.0f, 0.0f});
    auto light = childNode->CreateComponent<Light>();
    light->SetCastShadows(true);
    light->SetColor(Color::RED);

    auto prefab = MakeShared<XMLFile>(context);
    XMLElement prefabRootElement = prefab->CreateRoot("node");
    node->SaveXML(prefabRootElement);
    return prefab;
}

}

TEST_CASE("Time is synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Prepare test parameters
    const float frameErrorTolarance = 1.0f;
    const auto retry = GENERATE(range(0, 5));
    const auto quality = GENERATE(
        Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f },
        Tests::ConnectionQuality{ 0.24f, 0.28f, 0.50f, 0.10f, 0.10f }
    );
    const float averagePingSec = (quality.maxPing_ + quality.minPing_) / 2;

    const unsigned initialSyncTime = 20;
    const unsigned initialWaitTime = 40;
    const unsigned forwardSyncTime = 20;
    const unsigned forwardWaitTime = 40;
    const unsigned backwardSyncTime = 20;
    const unsigned backwardWaitTime = 40;

    unsigned seed = retry;
    CombineHash(seed, MakeHash(quality.minPing_));
    CombineHash(seed, MakeHash(quality.maxPing_));
    CombineHash(seed, MakeHash(quality.spikePing_));

    // Setup scenes
    auto serverScene = MakeShared<Scene>(context);
    auto clientScene = MakeShared<Scene>(context);

    Tests::NetworkSimulator sim(serverScene, seed);
    sim.AddClient(clientScene, quality);

    auto& serverNetworkManager = serverScene->GetNetworkManager()->AsServer();
    auto& clientNetworkManager = clientScene->GetNetworkManager()->AsClient();

    // Simulate a few millseconds, not enough for synchronization due to ping
    sim.SimulateTime(504 / 1024.0f);
    REQUIRE_FALSE(clientNetworkManager.IsSynchronized());

    // Simulate a few more seconds, should be somehow synchronized
    sim.SimulateTime(520 / 1024.0f);
    sim.SimulateTime(9.0f);

    REQUIRE(clientNetworkManager.IsSynchronized());
    REQUIRE(clientNetworkManager.GetPingMs() == RoundToInt(averagePingSec * 1000));

    const float syncError = ea::max(0.5f, (quality.maxPing_ - quality.minPing_) * Tests::NetworkSimulator::FramesInSecond);
    const auto startTime = 32 * 10;
    REQUIRE(serverNetworkManager.GetCurrentFrame() == startTime);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(startTime)) < syncError);

    // Simulate some time, should be precisely synchronized afterwards
    sim.SimulateTime(initialSyncTime);
    REQUIRE(serverNetworkManager.GetCurrentFrame() == startTime + 32 * initialSyncTime);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(startTime + 32 * initialSyncTime)) < frameErrorTolarance);

    // Simulate more time, expect time to stay synchronized
    const unsigned syncFrame1 = clientNetworkManager.GetLastSynchronizationFrame();
    sim.SimulateTime(initialWaitTime);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == startTime + 32 * (initialSyncTime + initialWaitTime));
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(startTime + 32 * (initialSyncTime + initialWaitTime))) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() == syncFrame1);

    // Warp time close to 2^32 and simulate some time, expect time to be resynchronized
    const auto bigTime = M_MAX_UNSIGNED - 32 * 30;
    serverNetworkManager.SetCurrentFrame(bigTime / 3);
    sim.SimulateTime(1.0f);
    serverNetworkManager.SetCurrentFrame(bigTime / 3 * 2);
    sim.SimulateTime(1.0f);
    serverNetworkManager.SetCurrentFrame(bigTime);
    sim.SimulateTime(forwardSyncTime);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == bigTime + 32 * forwardSyncTime);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(bigTime + 32 * forwardSyncTime)) < frameErrorTolarance);

    // Simulate more time, expect time to stay synchronized
    const unsigned syncFrame2 = clientNetworkManager.GetLastSynchronizationFrame();
    sim.SimulateTime(forwardWaitTime);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == bigTime + 32 * (forwardSyncTime + forwardWaitTime));
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(bigTime + 32 * (forwardSyncTime + forwardWaitTime))) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() == syncFrame2);

    // Warp time 1 second back and simulate some time, expect time to be resynchronized
    const unsigned baseTime = bigTime + 32 * (forwardSyncTime + forwardWaitTime);
    serverNetworkManager.SetCurrentFrame(baseTime - 32 * 1);
    sim.SimulateTime(backwardSyncTime + 1);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == baseTime + 32 * backwardSyncTime);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(baseTime + 32 * backwardSyncTime)) < frameErrorTolarance);

    // Simulate more time, expect time to stay synchronized
    const unsigned syncFrame3 = clientNetworkManager.GetLastSynchronizationFrame();
    sim.SimulateTime(backwardWaitTime);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == baseTime + 32 * (backwardSyncTime + backwardWaitTime));
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(baseTime + 32 * (backwardSyncTime + backwardWaitTime))) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() == syncFrame3);
}

TEST_CASE("Scene is synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);
    const float syncDelay = 0.25f;

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    // Reference transforms, should stay the same
    Matrix3x4 transformReplicatedNodeA;
    Matrix3x4 transformReplicatedNodeB;
    Matrix3x4 transformReplicatedNodeChild1;
    Matrix3x4 transformReplicatedNodeChild2;
    Matrix3x4 transformReplicatedNodeChild4;

    {
        for (Scene* clientScene : clientScenes)
            clientScene->CreateChild("Client Only Node");
        auto serverOnlyNode = serverScene->CreateChild("Server Only Node");

        auto replicatedNodeA = serverScene->CreateChild("Replicated Node A");
        replicatedNodeA->CreateComponent<DefaultNetworkObject>();
        replicatedNodeA->SetScale(2.0f);
        transformReplicatedNodeA = replicatedNodeA->GetWorldTransform();

        auto replicatedNodeB = serverScene->CreateChild("Replicated Node B");
        replicatedNodeB->CreateComponent<DefaultNetworkObject>();
        replicatedNodeB->SetPosition({ -1.0f, 2.0f, 0.5f });
        transformReplicatedNodeB = replicatedNodeB->GetWorldTransform();

        auto replicatedNodeChild1 = replicatedNodeA->CreateChild("Replicated Node Child 1");
        replicatedNodeChild1->CreateComponent<DefaultNetworkObject>();
        replicatedNodeChild1->SetPosition({ -2.0f, 3.0f, 1.5f });
        transformReplicatedNodeChild1 = replicatedNodeChild1->GetWorldTransform();

        auto replicatedNodeChild2 = replicatedNodeChild1->CreateChild("Replicated Node Child 2");
        replicatedNodeChild2->CreateComponent<DefaultNetworkObject>();
        replicatedNodeChild2->SetRotation({ 90.0f, Vector3::UP });
        transformReplicatedNodeChild2 = replicatedNodeChild2->GetWorldTransform();

        auto serverOnlyChild3 = replicatedNodeB->CreateChild("Server Only Child 3");
        serverOnlyChild3->SetPosition({ -1.0f, 0.0f, 0.0f });

        auto replicatedNodeChild4 = serverOnlyChild3->CreateChild("Replicated Node Child 4");
        replicatedNodeChild4->CreateComponent<DefaultNetworkObject>();
        transformReplicatedNodeChild4 = replicatedNodeChild4->GetWorldTransform();
    }

    // Spend some time alone
    Tests::NetworkSimulator sim(serverScene);
    sim.SimulateTime(10.0f);

    // Add clients and wait for synchronization
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);
    sim.SimulateTime(10.0f);

    for (Scene* clientScene : clientScenes)
    {
        auto clientOnlyNode = clientScene->GetChild("Client Only Node", true);
        auto replicatedNodeA = clientScene->GetChild("Replicated Node A", true);
        auto replicatedNodeB = clientScene->GetChild("Replicated Node B", true);
        auto replicatedNodeChild1 = clientScene->GetChild("Replicated Node Child 1", true);
        auto replicatedNodeChild2 = clientScene->GetChild("Replicated Node Child 2", true);
        auto replicatedNodeChild4 = clientScene->GetChild("Replicated Node Child 4", true);

        REQUIRE(clientScene->GetNumChildren() == 3);
        REQUIRE(clientScene == clientOnlyNode->GetParent());
        REQUIRE(clientScene == replicatedNodeA->GetParent());
        REQUIRE(clientScene == replicatedNodeB->GetParent());

        REQUIRE(clientOnlyNode->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetNumChildren() == 1);
        REQUIRE(replicatedNodeA == replicatedNodeChild1->GetParent());

        REQUIRE(replicatedNodeChild1->GetNumChildren() == 1);
        REQUIRE(replicatedNodeChild1 == replicatedNodeChild2->GetParent());

        REQUIRE(replicatedNodeChild2->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetNumChildren() == 1);
        REQUIRE(replicatedNodeB == replicatedNodeChild4->GetParent());

        REQUIRE(replicatedNodeChild4->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetWorldTransform().Equals(transformReplicatedNodeA));
        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2));
        REQUIRE(replicatedNodeChild4->GetWorldTransform().Equals(transformReplicatedNodeChild4));
    }

    // Re-parent "Server Only Child 3" to "Replicated Node A"
    // Re-parent "Replicated Node Child 1" to Scene
    // Wait for synchronization
    {
        auto serverOnlyChild3 = serverScene->GetChild("Server Only Child 3", true);
        auto replicatedNodeA = serverScene->GetChild("Replicated Node A", true);
        auto replicatedNodeChild1 = serverScene->GetChild("Replicated Node Child 1", true);

        serverOnlyChild3->SetParent(replicatedNodeA);
        replicatedNodeChild1->SetParent(serverScene);
    }

    sim.SimulateTime(syncDelay);

    for (Scene* clientScene : clientScenes)
    {
        auto clientOnlyNode = clientScene->GetChild("Client Only Node", true);
        auto replicatedNodeA = clientScene->GetChild("Replicated Node A", true);
        auto replicatedNodeB = clientScene->GetChild("Replicated Node B", true);
        auto replicatedNodeChild1 = clientScene->GetChild("Replicated Node Child 1", true);
        auto replicatedNodeChild2 = clientScene->GetChild("Replicated Node Child 2", true);
        auto replicatedNodeChild4 = clientScene->GetChild("Replicated Node Child 4", true);

        REQUIRE(clientScene->GetNumChildren() == 4);
        REQUIRE(clientScene == clientOnlyNode->GetParent());
        REQUIRE(clientScene == replicatedNodeA->GetParent());
        REQUIRE(clientScene == replicatedNodeB->GetParent());
        REQUIRE(clientScene == replicatedNodeChild1->GetParent());

        REQUIRE(clientOnlyNode->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetNumChildren() == 1);
        REQUIRE(replicatedNodeA == replicatedNodeChild4->GetParent());

        REQUIRE(replicatedNodeChild4->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetNumChildren() == 0);

        REQUIRE(replicatedNodeChild1->GetNumChildren() == 1);
        REQUIRE(replicatedNodeChild1 == replicatedNodeChild2->GetParent());

        REQUIRE(replicatedNodeChild2->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetWorldTransform().Equals(transformReplicatedNodeA));
        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2));
        REQUIRE(replicatedNodeChild4->GetWorldTransform().Equals(transformReplicatedNodeChild4));
    }

    // Remove "Replicated Node A"
    // Add "Replicated Node C"
    {
        auto replicatedNodeA = serverScene->GetChild("Replicated Node A", true);
        replicatedNodeA->Remove();
        auto replicatedNodeC = serverScene->CreateChild("Replicated Node C");
        replicatedNodeC->CreateComponent<DefaultNetworkObject>();
    }

    sim.SimulateTime(syncDelay);

    for (Scene* clientScene : clientScenes)
    {
        auto clientOnlyNode = clientScene->GetChild("Client Only Node", true);
        auto replicatedNodeB = clientScene->GetChild("Replicated Node B", true);
        auto replicatedNodeC = clientScene->GetChild("Replicated Node C", true);
        auto replicatedNodeChild1 = clientScene->GetChild("Replicated Node Child 1", true);
        auto replicatedNodeChild2 = clientScene->GetChild("Replicated Node Child 2", true);

        REQUIRE(clientScene->GetNumChildren() == 4);
        REQUIRE(clientScene == clientOnlyNode->GetParent());
        REQUIRE(clientScene == replicatedNodeB->GetParent());
        REQUIRE(clientScene == replicatedNodeC->GetParent());
        REQUIRE(clientScene == replicatedNodeChild1->GetParent());

        REQUIRE(clientOnlyNode->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetNumChildren() == 0);

        REQUIRE(replicatedNodeChild1->GetNumChildren() == 1);
        REQUIRE(replicatedNodeChild1 == replicatedNodeChild2->GetParent());

        REQUIRE(replicatedNodeChild2->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB));
        REQUIRE(replicatedNodeC->GetWorldTransform().Equals(Matrix3x4::IDENTITY));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2));
    }

    sim.SimulateTime(1.0f);
}

TEST_CASE("Position and rotation are synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0, 0 };
    const float moveSpeedNodeA = 1.0f;
    const float rotationSpeedNodeA = 10.0f;
    const float moveSpeedNodeB = 0.1f;
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    auto serverNodeA = serverScene->CreateChild("Node");
    auto serverObjectA = serverNodeA->CreateComponent<DefaultNetworkObject>();

    auto serverNodeB = serverNodeA->CreateChild("Node Child");
    auto serverObjectB = serverNodeB->CreateComponent<DefaultNetworkObject>();
    serverNodeB->SetPosition({ 0.0f, 0.0f, 1.0f });

    // Animate objects forever
    serverScene->SubscribeToEvent(serverScene, E_SCENEUPDATE, [&](StringHash, VariantMap& eventData)
    {
        const float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        serverNodeA->Translate(timeStep * moveSpeedNodeA * Vector3::LEFT, TS_PARENT);
        serverNodeA->Rotate({ timeStep * rotationSpeedNodeA, Vector3::UP }, TS_PARENT);
        serverNodeB->Translate(timeStep * moveSpeedNodeB * Vector3::FORWARD, TS_PARENT);
    });

    // Spend some time alone
    Tests::NetworkSimulator sim(serverScene);
    const auto& serverNetworkManager = serverScene->GetNetworkManager()->AsServer();
    sim.SimulateTime(9.0f);

    // Add clients and wait for synchronization
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);
    sim.SimulateTime(9.0f);

    // Expect positions and rotations to be precisely synchronized
    const float expectedDelay = 0.2;
    for (Scene* clientScene : clientScenes)
    {
        const NetworkTime clientTime = clientScene->GetNetworkManager()->AsClient().GetClientTime();
        const double delay = serverNetworkManager.GetServerTime() - clientTime;

        auto clientNodeA = clientScene->GetChild("Node", true);
        auto clientNodeB = clientScene->GetChild("Node Child", true);

        REQUIRE(delay / Tests::NetworkSimulator::FramesInSecond == Catch::Approx(expectedDelay).margin(0.02));

        REQUIRE(serverObjectA->GetTemporalWorldPosition(clientTime).Equals(clientNodeA->GetWorldPosition(), M_LARGE_EPSILON));
        REQUIRE(serverObjectA->GetTemporalWorldRotation(clientTime).Equals(clientNodeA->GetWorldRotation(), M_LARGE_EPSILON));

        REQUIRE(serverObjectB->GetTemporalWorldPosition(clientTime).Equals(clientNodeB->GetWorldPosition(), M_LARGE_EPSILON));
        REQUIRE(serverObjectB->GetTemporalWorldRotation(clientTime).Equals(clientNodeB->GetWorldRotation(), M_LARGE_EPSILON));
    }
}

TEST_CASE("Prefabs are replicated on clients")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<XMLFile>(
        context, "@/SceneSynchronization/TestPrefab.xml", [&] { return CreateTestPrefab(context); });

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    // Start simulation
    Tests::NetworkSimulator sim(serverScene);
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);

    // Create nodes
    {
        Node* node1 = serverScene->CreateChild("Node 1");
        node1->SetPosition({1.0f, 0.0f, 0.0f});
        auto object1 = node1->CreateComponent<DefaultNetworkObject>();
        object1->SetClientPrefab(prefab);

        Node* node2 = serverScene->CreateChild("Node 2");
        node2->SetPosition({2.0f, 0.0f, 0.0f});
        auto object2 = node2->CreateComponent<DefaultNetworkObject>();
        object2->SetClientPrefab(prefab);
    }
    sim.SimulateTime(10.0f);

    // Expect prefabs replicated
    for (Scene* clientScene : clientScenes)
    {
        auto node1 = clientScene->GetChild("Node 1", true);
        REQUIRE(node1);

        auto node2 = clientScene->GetChild("Node 2", true);
        REQUIRE(node2);

        auto node1Child = node1->GetChild("Child");
        REQUIRE(node1Child);

        auto node2Child = node2->GetChild("Child");
        REQUIRE(node2Child);

        CHECK(node1->GetWorldPosition().Equals({1.0f, 0.0f, 0.0f}));
        CHECK(node1Child->GetWorldPosition().Equals({1.0f, 1.0f, 0.0f}));
        CHECK(node2->GetWorldPosition().Equals({2.0f, 0.0f, 0.0f}));
        CHECK(node2Child->GetWorldPosition().Equals({2.0f, 1.0f, 0.0f}));

        auto staticModel1 = node1->GetComponent<StaticModel>();
        REQUIRE(staticModel1);

        auto staticModel2 = node2->GetComponent<StaticModel>();
        REQUIRE(staticModel2);

        auto light1 = node1Child->GetComponent<Light>();
        REQUIRE(light1);

        auto light2 = node2Child->GetComponent<Light>();
        REQUIRE(light2);

        CHECK(staticModel1->GetCastShadows() == true);
        CHECK(staticModel2->GetCastShadows() == true);
        CHECK(light1->GetCastShadows() == true);
        CHECK(light2->GetCastShadows() == true);
        CHECK(light1->GetColor() == Color::RED);
        CHECK(light2->GetColor() == Color::RED);
    }
}

TEST_CASE("Ownership is consistent on server and on clients")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    // Start simulation
    Tests::NetworkSimulator sim(serverScene);
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);

    // Create nodes
    {
        Node* node = serverScene->CreateChild("Unowned Node");
        auto object = node->CreateComponent<DefaultNetworkObject>();
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Draft);
    }
    {
        Node* node = serverScene->CreateChild("Owned Node 0");
        auto object = node->CreateComponent<DefaultNetworkObject>();
        object->SetOwner(sim.GetServerToClientConnection(clientScenes[0]));
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Draft);
    }
    {
        Node* node = serverScene->CreateChild("Owned Node 1");
        auto object = node->CreateComponent<DefaultNetworkObject>();
        object->SetOwner(sim.GetServerToClientConnection(clientScenes[1]));
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Draft);
    }
    {
        Node* node = serverScene->CreateChild("Owned Node 2");
        auto object = node->CreateComponent<DefaultNetworkObject>();
        object->SetOwner(sim.GetServerToClientConnection(clientScenes[2]));
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Draft);
    }
    sim.SimulateTime(10.0f);

    // Check ownership
    const auto getObject = [](Scene* scene, const ea::string& name)
    {
        return scene->GetChild(name, true)->GetDerivedComponent<NetworkObject>();
    };

    REQUIRE(getObject(serverScene, "Unowned Node")->GetNetworkMode() == NetworkObjectMode::Server);
    REQUIRE(getObject(serverScene, "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::Server);
    REQUIRE(getObject(serverScene, "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::Server);
    REQUIRE(getObject(serverScene, "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::Server);

    REQUIRE(getObject(clientScenes[0], "Unowned Node")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[0], "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::ClientOwned);
    REQUIRE(getObject(clientScenes[0], "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[0], "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);

    REQUIRE(getObject(clientScenes[1], "Unowned Node")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[1], "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[1], "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::ClientOwned);
    REQUIRE(getObject(clientScenes[1], "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);

    REQUIRE(getObject(clientScenes[2], "Unowned Node")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[2], "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[2], "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[2], "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::ClientOwned);
}

TEST_CASE("Physics is synchronized with network updates")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Simulate some time before scene creation so network is not synchronized with scene
    Tests::NetworkSimulator::SimulateEngineFrame(context, 0.01234f);

    // Start simulation and track events
    auto serverScene = MakeShared<Scene>(context);
    auto serverPhysicsWorld = serverScene->CreateComponent<PhysicsWorld>();
    serverPhysicsWorld->SetFps(64);

    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    Tests::NetworkSimulator sim(serverScene);

    sim.SimulateTime(1.0f);

    // Add client and wait for synchronization
    SharedPtr<Scene> clientScene = MakeShared<Scene>(context);
    auto clientPhysicsWorld = clientScene->CreateComponent<PhysicsWorld>();
    clientPhysicsWorld->SetFps(64);

    sim.AddClient(clientScene, quality);
    sim.SimulateTime(10.0f);

    // Expect to have alternating frames:
    // - ...
    // - (end frame)
    // - E_PHYSICSPRESTEP
    // - (end frame)
    // - E_PHYSICSPRESTEP
    // - E_NETWORKUPDATE
    // - (end frame)
    // - E_PHYSICSPRESTEP
    // - (end frame)
    // - E_PHYSICSPRESTEP
    // - E_NETWORKUPDATE
    // - (end frame)
    // - ...

    auto serverEventTracker = MakeShared<Tests::FrameEventTracker>(context);
    serverEventTracker->TrackEvent(serverPhysicsWorld, E_PHYSICSPRESTEP);
    serverEventTracker->TrackEvent(E_NETWORKUPDATE);

    auto clientEventTracker = MakeShared<Tests::FrameEventTracker>(context);
    clientEventTracker->TrackEvent(clientPhysicsWorld, E_PHYSICSPRESTEP);
    clientEventTracker->TrackEvent(E_NETWORKCLIENTUPDATE);

    sim.SimulateTime(1.0f);
    serverEventTracker->SkipFramesUntilEvent(E_NETWORKUPDATE);
    clientEventTracker->SkipFramesUntilEvent(E_NETWORKCLIENTUPDATE);

    REQUIRE(serverEventTracker->GetNumFrames() > 4);
    REQUIRE(clientEventTracker->GetNumFrames() > 4);

    serverEventTracker->ValidatePattern({{E_PHYSICSPRESTEP, E_NETWORKUPDATE}, {}, {E_PHYSICSPRESTEP}, {}});
    clientEventTracker->ValidatePattern({{E_NETWORKCLIENTUPDATE, E_PHYSICSPRESTEP}, {}, {E_PHYSICSPRESTEP}, {}});
}
