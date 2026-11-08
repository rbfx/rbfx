// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "../CommonUtils.h"

#include <Urho3D/Core/Exception.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/ModelView.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/RenderPipeline/ShaderConsts.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Utility/GLTFImporter.h>

#include <cmath>
#include <thread>

namespace
{

class CaptureImporterCallback : public GLTFImporterCallback
{
public:
    bool failOnModel_{};
    unsigned numVertices_{};
    unsigned numIndices_{};
    unsigned numBones_{};
    bool windingMatchesNormal_{};
    bool materialWasCached_{};
    Vector3 tangent_;
    ea::vector<PrimitiveType> primitiveTypes_;
    ea::vector<ModelVertexMorph> vertexMorphs_;
    ea::string materialName_;
    ea::vector<ea::string> artificialSkinNodes_;
    ResourceCache* resourceCache_{};
    ea::array<ea::vector<float>, 2> inTangents_;
    ea::array<ea::vector<float>, 2> outTangents_;
    ea::vector<ea::string> variantTrackNames_;
    ea::unordered_map<ea::string, ea::vector<AnimationKeyFrame>> boneKeyFrames_;

    void OnModelLoaded(ModelView& modelView) override
    {
        numBones_ = modelView.GetBones().size();
        if (modelView.GetGeometries().empty() || modelView.GetGeometries()[0].lods_.empty())
            return;
        for (const GeometryView& geometry : modelView.GetGeometries())
        {
            for (const GeometryLODView& geometryLOD : geometry.lods_)
            {
                primitiveTypes_.push_back(geometryLOD.primitiveType_);
                if (const auto iter = geometryLOD.morphs_.find(0); iter != geometryLOD.morphs_.end())
                    vertexMorphs_.insert(vertexMorphs_.end(), iter->second.begin(), iter->second.end());
            }
        }
        const GeometryLODView& lod = modelView.GetGeometries()[0].lods_[0];
        numVertices_ = lod.vertices_.size();
        numIndices_ = lod.indices_.size();
        materialName_ = modelView.GetGeometries()[0].material_;
        materialWasCached_ =
            resourceCache_ && !materialName_.empty() && resourceCache_->GetExistingResource<Material>(materialName_);
        if (!lod.vertices_.empty())
            tangent_ = lod.vertices_[0].GetTangent();
        if (lod.indices_.size() >= 3)
        {
            const ModelVertex& v0 = lod.vertices_[lod.indices_[0]];
            const ModelVertex& v1 = lod.vertices_[lod.indices_[1]];
            const ModelVertex& v2 = lod.vertices_[lod.indices_[2]];
            const Vector3 faceNormal =
                (v1.GetPosition() - v0.GetPosition()).CrossProduct(v2.GetPosition() - v0.GetPosition());
            windingMatchesNormal_ = faceNormal.DotProduct(v0.GetNormal()) > 0.0f;
        }
        if (failOnModel_)
            throw RuntimeException("Expected importer callback failure");
    }

    void OnAnimationLoaded(Animation& animation) override
    {
        for (const auto& [_, track] : animation.GetTracks())
            boneKeyFrames_[track.name_] = track.keyFrames_;
        for (const auto& [_, track] : animation.GetVariantTracks())
        {
            variantTrackNames_.push_back(track.name_);
            const unsigned morphIndex = track.name_.ends_with("/Morphs/0") ? 0 : 1;
            for (const Variant& tangent : track.inTangents_)
                inTangents_[morphIndex].push_back(tangent.GetFloat());
            for (const Variant& tangent : track.outTangents_)
                outTangents_[morphIndex].push_back(tangent.GetFloat());
        }
    }

    ea::vector<ea::string> GetArtificialSkinNodes() override { return artificialSkinNodes_; }
};

GLTFImporterSettings GetTestSettings()
{
    GLTFImporterSettings settings;
    settings.cleanupRootNodes_ = false;
    settings.preview_.addLights_ = false;
    settings.preview_.addSkybox_ = false;
    settings.preview_.addReflectionProbe_ = false;
    settings.preview_.highRenderQuality_ = false;
    return settings;
}

} // namespace

TEST_CASE("glTF importer preserves handedness and cubic morph tangents")
{
    static const ea::string gltf = R"({
        "asset":{"version":"2.0"},
        "buffers":[{"byteLength":208,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAABAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAPwAAIEEAADBBzczMPc3MTD4AAKBBAACoQQAA8EEAAPhBmpmZPs3MzD4AACBCAAAkQg=="}],
        "bufferViews":[
            {"buffer":0,"byteOffset":0,"byteLength":36,"target":34962},
            {"buffer":0,"byteOffset":36,"byteLength":36,"target":34962},
            {"buffer":0,"byteOffset":72,"byteLength":6,"target":34963},
            {"buffer":0,"byteOffset":80,"byteLength":36},
            {"buffer":0,"byteOffset":116,"byteLength":36},
            {"buffer":0,"byteOffset":152,"byteLength":8},
            {"buffer":0,"byteOffset":160,"byteLength":48}
        ],
        "accessors":[
            {"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},
            {"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"},
            {"bufferView":2,"componentType":5123,"count":3,"type":"SCALAR"},
            {"bufferView":3,"componentType":5126,"count":3,"type":"VEC3"},
            {"bufferView":4,"componentType":5126,"count":3,"type":"VEC3"},
            {"bufferView":5,"componentType":5126,"count":2,"type":"SCALAR","min":[0],"max":[1]},
            {"bufferView":6,"componentType":5126,"count":12,"type":"SCALAR"}
        ],
        "meshes":[{"name":"Triangle","weights":[0,0],"primitives":[{
            "attributes":{"POSITION":0,"NORMAL":1},"indices":2,
            "targets":[{"POSITION":3},{"POSITION":4}]
        }]}],
        "nodes":[{"name":"Mirrored","mesh":0,"scale":[1,-1,1]}],
        "animations":[{"name":"Morph","samplers":[{
            "input":5,"output":6,"interpolation":"CUBICSPLINE"
        }],"channels":[{"sampler":0,"target":{"node":0,"path":"weights"}}]}],
        "scenes":[{"nodes":[0]}],"scene":0
    })";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fileSystem = context->GetSubsystem<FileSystem>();
    TemporaryDir temporaryDir{context, fileSystem->GetTemporaryDir() + "rbfx-model-importer-tests"};
    const ea::string fileName = temporaryDir.GetPath() + "handedness.gltf";
    File file{context, fileName, FILE_WRITE};
    REQUIRE(file.Write(gltf.data(), gltf.size()) == gltf.size());
    file.Close();

    CaptureImporterCallback callback;
    GLTFImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFile(fileName));
    REQUIRE(importer.Process(temporaryDir.GetPath(), "Test/", &callback));

    CHECK(callback.windingMatchesNormal_);
    CHECK(callback.inTangents_[0] == ea::vector<float>{10.0f, 30.0f});
    CHECK(callback.outTangents_[0] == ea::vector<float>{20.0f, 40.0f});
    CHECK(callback.inTangents_[1] == ea::vector<float>{11.0f, 31.0f});
    CHECK(callback.outTangents_[1] == ea::vector<float>{21.0f, 41.0f});
}

TEST_CASE("glTF importer scales cubic morph tangents by key frame segment duration")
{
    // Keys are 0, 0.5 and 2, so the segments are 0.5 and 1.5 seconds long.
    // Each key has in-tangents {4, 8}, values {0.25, 0.5} and out-tangents {12, 16}.
    static const ea::string gltf = R"({
        "asset":{"version":"2.0"},
        "buffers":[{"byteLength":236,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAABAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPwAAAEAAAIBAAAAAQQAAgD4AAAA/AABAQQAAgEEAAIBAAAAAQQAAgD4AAAA/AABAQQAAgEEAAIBAAAAAQQAAgD4AAAA/AABAQQAAgEE="}],
        "bufferViews":[
            {"buffer":0,"byteOffset":0,"byteLength":36,"target":34962},
            {"buffer":0,"byteOffset":36,"byteLength":36,"target":34962},
            {"buffer":0,"byteOffset":72,"byteLength":6,"target":34963},
            {"buffer":0,"byteOffset":80,"byteLength":36},
            {"buffer":0,"byteOffset":116,"byteLength":36},
            {"buffer":0,"byteOffset":152,"byteLength":12},
            {"buffer":0,"byteOffset":164,"byteLength":72}
        ],
        "accessors":[
            {"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},
            {"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"},
            {"bufferView":2,"componentType":5123,"count":3,"type":"SCALAR"},
            {"bufferView":3,"componentType":5126,"count":3,"type":"VEC3"},
            {"bufferView":4,"componentType":5126,"count":3,"type":"VEC3"},
            {"bufferView":5,"componentType":5126,"count":3,"type":"SCALAR","min":[0],"max":[2]},
            {"bufferView":6,"componentType":5126,"count":18,"type":"SCALAR"}
        ],
        "meshes":[{"name":"Triangle","weights":[0,0],"primitives":[{
            "attributes":{"POSITION":0,"NORMAL":1},"indices":2,
            "targets":[{"POSITION":3},{"POSITION":4}]
        }]}],
        "nodes":[{"name":"Morphed","mesh":0}],
        "animations":[{"name":"Morph","samplers":[{
            "input":5,"output":6,"interpolation":"CUBICSPLINE"
        }],"channels":[{"sampler":0,"target":{"node":0,"path":"weights"}}]}],
        "scenes":[{"nodes":[0]}],"scene":0
    })";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fileSystem = context->GetSubsystem<FileSystem>();
    TemporaryDir temporaryDir{context, fileSystem->GetTemporaryDir() + "rbfx-model-importer-tests"};
    const ea::string fileName = temporaryDir.GetPath() + "tangent-scale.gltf";
    File file{context, fileName, FILE_WRITE};
    REQUIRE(file.Write(gltf.data(), gltf.size()) == gltf.size());
    file.Close();

    CaptureImporterCallback callback;
    GLTFImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFile(fileName));
    REQUIRE(importer.Process(temporaryDir.GetPath(), "Test/", &callback));

    CHECK(callback.inTangents_[0] == ea::vector<float>{2.0f, 2.0f, 6.0f});
    CHECK(callback.outTangents_[0] == ea::vector<float>{6.0f, 18.0f, 18.0f});
    CHECK(callback.inTangents_[1] == ea::vector<float>{4.0f, 4.0f, 12.0f});
    CHECK(callback.outTangents_[1] == ea::vector<float>{8.0f, 24.0f, 24.0f});
}

TEST_CASE("glTF importer samples cubic spline bone animation")
{
    // The bone moves from 0 to 1 on X in 1 second.
    // The out-tangent of the first key is 3, so the spline at 0.5 s is at 0.875, not at 0.5.
    // The importer mirrors X to convert glTF to left-handed space, so the expected X values are negative.
    static const ea::string gltf = R"({
        "asset":{"version":"2.0"},
        "buffers":[{"byteLength":220,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAABAAIAAAAAAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAAAAAAAAAA=="}],
        "bufferViews":[
            {"buffer":0,"byteOffset":0,"byteLength":36,"target":34962},
            {"buffer":0,"byteOffset":36,"byteLength":36,"target":34962},
            {"buffer":0,"byteOffset":72,"byteLength":6,"target":34963},
            {"buffer":0,"byteOffset":80,"byteLength":12,"target":34962},
            {"buffer":0,"byteOffset":92,"byteLength":48,"target":34962},
            {"buffer":0,"byteOffset":140,"byteLength":8},
            {"buffer":0,"byteOffset":148,"byteLength":72}
        ],
        "accessors":[
            {"bufferView":0,"componentType":5126,"count":3,"type":"VEC3","min":[0,0,0],"max":[1,1,0]},
            {"bufferView":1,"componentType":5126,"count":3,"type":"VEC3"},
            {"bufferView":2,"componentType":5123,"count":3,"type":"SCALAR"},
            {"bufferView":3,"componentType":5121,"count":3,"type":"VEC4"},
            {"bufferView":4,"componentType":5126,"count":3,"type":"VEC4"},
            {"bufferView":5,"componentType":5126,"count":2,"type":"SCALAR","min":[0],"max":[1]},
            {"bufferView":6,"componentType":5126,"count":6,"type":"VEC3"}
        ],
        "meshes":[{"name":"Triangle","primitives":[{
            "attributes":{"POSITION":0,"NORMAL":1,"JOINTS_0":3,"WEIGHTS_0":4},"indices":2
        }]}],
        "skins":[{"joints":[1]}],
        "nodes":[{"name":"Skinned","mesh":0,"skin":0},{"name":"Bone"}],
        "animations":[{"name":"Move","samplers":[{
            "input":5,"output":6,"interpolation":"CUBICSPLINE"
        }],"channels":[{"sampler":0,"target":{"node":1,"path":"translation"}}]}],
        "scenes":[{"nodes":[0,1]}],"scene":0
    })";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fileSystem = context->GetSubsystem<FileSystem>();
    TemporaryDir temporaryDir{context, fileSystem->GetTemporaryDir() + "rbfx-model-importer-tests"};
    const ea::string fileName = temporaryDir.GetPath() + "spline-bone.gltf";
    File file{context, fileName, FILE_WRITE};
    REQUIRE(file.Write(gltf.data(), gltf.size()) == gltf.size());
    file.Close();

    const auto importBoneKeyFrames = [&](float sampleRate)
    {
        GLTFImporterSettings settings = GetTestSettings();
        settings.splineSampleRate_ = sampleRate;
        CaptureImporterCallback callback;
        GLTFImporter importer{context, settings};
        REQUIRE(importer.LoadFile(fileName));
        REQUIRE(importer.Process(temporaryDir.GetPath(), "Test/", &callback));
        REQUIRE(callback.boneKeyFrames_.contains("Bone"));
        return callback.boneKeyFrames_["Bone"];
    };

    const auto sampled = importBoneKeyFrames(2.0f);
    REQUIRE(sampled.size() == 3);
    CHECK(sampled[0].time_ == 0.0f);
    CHECK(sampled[1].time_ == 0.5f);
    CHECK(sampled[2].time_ == 1.0f);
    CHECK(sampled[0].position_.Equals(Vector3::ZERO));
    CHECK(sampled[1].position_.Equals(Vector3{-0.875f, 0.0f, 0.0f}));
    CHECK(sampled[2].position_.Equals(Vector3{-1.0f, 0.0f, 0.0f}));

    const auto unsampled = importBoneKeyFrames(0.0f);
    REQUIRE(unsampled.size() == 2);
    CHECK(unsampled[0].position_.Equals(Vector3::ZERO));
    CHECK(unsampled[1].position_.Equals(Vector3{-1.0f, 0.0f, 0.0f}));
}
