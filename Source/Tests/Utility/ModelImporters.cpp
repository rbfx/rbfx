// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "../CommonUtils.h"

#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/ModelView.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Utility/GLTFImporter.h>

namespace
{

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

/// Import glTF text from memory. The imported resources stay in the resource cache while the importer is alive.
SharedPtr<GLTFImporter> ImportGLTF(Context* context, const ea::string& gltf, const GLTFImporterSettings& settings)
{
    // The importer loads only the binary format from memory, so put the text in a GLB container.
    ea::string json = gltf;
    json.resize((json.size() + 3) / 4 * 4, ' ');

    VectorBuffer glb;
    glb.WriteUInt(0x46546C67); // "glTF"
    glb.WriteUInt(2);
    glb.WriteUInt(12 + 8 + json.size());
    glb.WriteUInt(json.size());
    glb.WriteUInt(0x4E4F534A); // "JSON"
    glb.Write(json.data(), json.size());

    auto importer = MakeShared<GLTFImporter>(context, settings);
    REQUIRE(importer->LoadFileBinary(glb.GetBuffer()));
    REQUIRE(importer->Process("", "Test/", nullptr));
    return importer;
}

const VariantAnimationTrack* FindVariantTrack(const Animation& animation, const ea::string& nameSuffix)
{
    for (const auto& [_, track] : animation.GetVariantTracks())
    {
        if (track.name_.ends_with(nameSuffix))
            return &track;
    }
    return nullptr;
}

ea::vector<float> ToFloats(const ea::vector<Variant>& values)
{
    ea::vector<float> result;
    for (const Variant& value : values)
        result.push_back(value.GetFloat());
    return result;
}

/// Import glTF text with the given spline sample rate. Return the key frames of the "Bone" track.
ea::vector<AnimationKeyFrame> ImportBoneKeyFrames(Context* context, const ea::string& gltf, float sampleRate)
{
    GLTFImporterSettings settings = GetTestSettings();
    settings.splineSampleRate_ = sampleRate;
    const auto importer = ImportGLTF(context, gltf, settings);

    auto cache = context->GetSubsystem<ResourceCache>();
    auto animation = cache->GetExistingResource<Animation>("Test/Animations/Move.ani");
    REQUIRE(animation);
    const AnimationTrack* track = animation->GetTrack(ea::string{"Bone"});
    REQUIRE(track);
    return track->keyFrames_;
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
    auto cache = context->GetSubsystem<ResourceCache>();
    const auto importer = ImportGLTF(context, gltf, GetTestSettings());

    auto model = cache->GetExistingResource<Model>("Test/Models/Triangle.mdl");
    REQUIRE(model);
    auto modelView = MakeShared<ModelView>(context);
    REQUIRE(modelView->ImportModel(model));
    REQUIRE(modelView->GetGeometries().size() == 1);
    REQUIRE(modelView->GetGeometries()[0].lods_.size() == 1);

    // The winding of the first triangle agrees with the vertex normal.
    const GeometryLODView& lod = modelView->GetGeometries()[0].lods_[0];
    REQUIRE(lod.indices_.size() == 3);
    const ModelVertex& v0 = lod.vertices_[lod.indices_[0]];
    const ModelVertex& v1 = lod.vertices_[lod.indices_[1]];
    const ModelVertex& v2 = lod.vertices_[lod.indices_[2]];
    const Vector3 faceNormal = (v1.GetPosition() - v0.GetPosition()).CrossProduct(v2.GetPosition() - v0.GetPosition());
    CHECK(faceNormal.DotProduct(v0.GetNormal()) > 0.0f);

    auto animation = cache->GetExistingResource<Animation>("Test/Animations/Morph.ani");
    REQUIRE(animation);
    const VariantAnimationTrack* morph0 = FindVariantTrack(*animation, "/Morphs/0");
    const VariantAnimationTrack* morph1 = FindVariantTrack(*animation, "/Morphs/1");
    REQUIRE(morph0);
    REQUIRE(morph1);
    CHECK(ToFloats(morph0->inTangents_) == ea::vector<float>{10.0f, 30.0f});
    CHECK(ToFloats(morph0->outTangents_) == ea::vector<float>{20.0f, 40.0f});
    CHECK(ToFloats(morph1->inTangents_) == ea::vector<float>{11.0f, 31.0f});
    CHECK(ToFloats(morph1->outTangents_) == ea::vector<float>{21.0f, 41.0f});
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
    auto cache = context->GetSubsystem<ResourceCache>();
    const auto importer = ImportGLTF(context, gltf, GetTestSettings());

    auto animation = cache->GetExistingResource<Animation>("Test/Animations/Morph.ani");
    REQUIRE(animation);
    const VariantAnimationTrack* morph0 = FindVariantTrack(*animation, "/Morphs/0");
    const VariantAnimationTrack* morph1 = FindVariantTrack(*animation, "/Morphs/1");
    REQUIRE(morph0);
    REQUIRE(morph1);
    CHECK(ToFloats(morph0->inTangents_) == ea::vector<float>{2.0f, 2.0f, 6.0f});
    CHECK(ToFloats(morph0->outTangents_) == ea::vector<float>{6.0f, 18.0f, 18.0f});
    CHECK(ToFloats(morph1->inTangents_) == ea::vector<float>{4.0f, 4.0f, 12.0f});
    CHECK(ToFloats(morph1->outTangents_) == ea::vector<float>{8.0f, 24.0f, 24.0f});
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

    const auto sampled = ImportBoneKeyFrames(context, gltf, 2.0f);
    REQUIRE(sampled.size() == 3);
    CHECK(sampled[0].time_ == 0.0f);
    CHECK(sampled[1].time_ == 0.5f);
    CHECK(sampled[2].time_ == 1.0f);
    CHECK(sampled[0].position_.Equals(Vector3::ZERO));
    CHECK(sampled[1].position_.Equals(Vector3{-0.875f, 0.0f, 0.0f}));
    CHECK(sampled[2].position_.Equals(Vector3{-1.0f, 0.0f, 0.0f}));

    const auto unsampled = ImportBoneKeyFrames(context, gltf, 0.0f);
    REQUIRE(unsampled.size() == 2);
    CHECK(unsampled[0].position_.Equals(Vector3::ZERO));
    CHECK(unsampled[1].position_.Equals(Vector3{-1.0f, 0.0f, 0.0f}));
}
