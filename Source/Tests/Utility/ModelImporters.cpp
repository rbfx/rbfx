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
#include <Urho3D/Utility/FBXImporter.h>
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

ea::string CreateOversizedSkinFBX(unsigned numBones)
{
    ea::string vertices;
    ea::string polygonIndices;
    ea::string objects;
    ea::string connections;
    for (unsigned i = 0; i < numBones; ++i)
    {
        vertices += Format("{}{},{},0", i == 0 ? "" : ",", i % 3, i / 3);
        const int polygonIndex = i % 3 == 2 ? -static_cast<int>(i) - 1 : static_cast<int>(i);
        polygonIndices += Format("{}{}", i == 0 ? "" : ",", polygonIndex);

        const unsigned boneId = 1000 + i;
        const unsigned clusterId = 2000 + i;
        objects += Format(R"(
    Model: {}, "Model::Bone{}", "LimbNode" {{
    }}
    Deformer: {}, "SubDeformer::Bone{}", "Cluster" {{
        Version: 100
        Indexes: *1 {{ a: {} }}
        Weights: *1 {{ a: 1 }}
        Transform: *16 {{ a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }}
        TransformLink: *16 {{ a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }}
    }}
)",
            boneId, i, clusterId, i, i);
        connections += Format(
            "    C: \"OO\",{},4\n    C: \"OO\",{},{}\n    C: \"OO\",{},3\n", clusterId, boneId, clusterId, boneId);
    }

    return Format(R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {{
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}}
Objects:  {{
    Geometry: 1, "Geometry::Oversized", "Mesh" {{
        Vertices: *{} {{ a: {} }}
        PolygonVertexIndex: *{} {{ a: {} }}
    }}
    Model: 2, "Model::Oversized", "Mesh" {{
        Shading: T
        Culling: "CullingOff"
    }}
    Model: 3, "Model::Root", "LimbNode" {{
    }}
    Deformer: 4, "Deformer::Skin", "Skin" {{
        Version: 101
    }}
{}
}}
Connections:  {{
    C: "OO",1,2
    C: "OO",4,1
    C: "OO",2,0
    C: "OO",3,0
{}
}}
)",
        numBones * 3, vertices, numBones, polygonIndices, objects, connections);
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

TEST_CASE("FBX importer reuses indexed vertices and builds artificial skin")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Quad", "Mesh" {
        Vertices: *12 {
            a: 0,0,0,1,0,0,1,1,0,0,1,0
        }
        PolygonVertexIndex: *6 {
            a: 0,1,-3,0,2,-4
        }
    }
    Model: 2, "Model::Quad", "Mesh" {
        Version: 232
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    callback.artificialSkinNodes_ = {"Quad"};
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Test/", &callback));

    CHECK(callback.numVertices_ == 4);
    CHECK(callback.numIndices_ == 6);
    CHECK(callback.numBones_ == 1);
}

TEST_CASE("FBX importer ignores unweighted skin clusters")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Model: 3, "Model::Used", "LimbNode" {
    }
    Model: 4, "Model::Unused", "LimbNode" {
    }
    Deformer: 5, "Deformer::Skin", "Skin" {
        Version: 101
    }
    Deformer: 6, "SubDeformer::Used", "Cluster" {
        Version: 100
        Indexes: *3 { a: 0,1,2 }
        Weights: *3 { a: 1,1,1 }
        Transform: *16 { a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }
        TransformLink: *16 { a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }
    }
    Deformer: 7, "SubDeformer::Unused", "Cluster" {
        Version: 100
        Indexes: *0 { a: }
        Weights: *0 { a: }
        Transform: *16 { a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }
        TransformLink: *16 { a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",5,1
    C: "OO",6,5
    C: "OO",7,5
    C: "OO",3,6
    C: "OO",4,7
    C: "OO",4,3
    C: "OO",2,0
    C: "OO",3,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Skin/", &callback));

    CHECK(callback.numBones_ == 1);
}

TEST_CASE("FBX importer keeps unweighted skin vertices on the mesh node")
{
    // The bone moves 1 meter up from the bind pose. Only vertex 0 has a weight.
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
GlobalSettings:  {
    Version: 1000
    Properties70:  {
        P: "UnitScaleFactor", "double", "Number", "",100
    }
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Model: 3, "Model::Bone", "LimbNode" {
        Properties70:  {
            P: "Lcl Translation", "Lcl Translation", "", "A",0,1,0
        }
    }
    Deformer: 5, "Deformer::Skin", "Skin" {
        Version: 101
    }
    Deformer: 6, "SubDeformer::Bone", "Cluster" {
        Version: 100
        Indexes: *1 { a: 0 }
        Weights: *1 { a: 1 }
        Transform: *16 { a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }
        TransformLink: *16 { a: 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1 }
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",5,1
    C: "OO",6,5
    C: "OO",3,6
    C: "OO",2,0
    C: "OO",3,0
}
)";

    struct SkinCallback : public GLTFImporterCallback
    {
        ea::vector<Vector3> sourcePositions_;
        ea::vector<Vector3> skinnedPositions_;

        void OnModelLoaded(ModelView& modelView) override
        {
            const ea::vector<BoneView>& bones = modelView.GetBones();
            ea::vector<Matrix3x4> boneWorld(bones.size());
            for (unsigned i = 0; i < bones.size(); ++i)
            {
                // The parent bone comes before the child bone.
                const Matrix3x4 local{bones[i].position_, bones[i].rotation_, bones[i].scale_};
                boneWorld[i] = bones[i].parentIndex_ < i ? boneWorld[bones[i].parentIndex_] * local : local;
            }

            for (const ModelVertex& vertex : modelView.GetGeometries()[0].lods_[0].vertices_)
            {
                Vector3 position;
                for (const auto& [index, weight] : vertex.GetBlendIndicesAndWeights())
                    position += (boneWorld[index] * bones[index].offsetMatrix_) * vertex.GetPosition() * weight;
                sourcePositions_.push_back(vertex.GetPosition());
                skinnedPositions_.push_back(position);
            }
        }
    };

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    SkinCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Skin/", &callback));

    REQUIRE(callback.skinnedPositions_.size() == 3);
    for (unsigned i = 0; i < 3; ++i)
    {
        // Vertex 0 moves with the bone. The unweighted vertices stay in the mesh space.
        const Vector3& source = callback.sourcePositions_[i];
        const Vector3 expected = source.Equals(Vector3::ZERO) ? source + Vector3::UP : source;
        INFO("source " << source.ToString().c_str() << " skinned " << callback.skinnedPositions_[i].ToString().c_str());
        CHECK(callback.skinnedPositions_[i].Equals(expected, 1e-4f));
    }
}

TEST_CASE("FBX importer detects LOD levels from node names")
{
    // The node names have the LOD suffix. The mesh names do not.
    // The numbers 0, 1, 2 and 1, 2, 3 are level indices. The importer computes the distances from the model size.
    // Other numbers are LOD distances. The importer keeps them.
    struct NameCase
    {
        ea::string suffixes_[3];
        bool indices_;
    };
    const NameCase nameCase = GENERATE(NameCase{{"0", "1", "2"}, true}, NameCase{{"1", "2", "3"}, true},
        NameCase{{"0", "10", "25.5"}, false}, NameCase{{"0", "2", "3"}, false});

    const ea::string fbx = Format(R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {{
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}}
Objects:  {{
    Geometry: 1, "Geometry::MeshA", "Mesh" {{
        Vertices: *9 {{ a: 0,0,0,1,0,0,0,1,0 }}
        PolygonVertexIndex: *3 {{ a: 0,1,-3 }}
    }}
    Geometry: 2, "Geometry::MeshB", "Mesh" {{
        Vertices: *9 {{ a: 0,0,0,1,0,0,0,1,0 }}
        PolygonVertexIndex: *3 {{ a: 0,1,-3 }}
    }}
    Geometry: 5, "Geometry::MeshC", "Mesh" {{
        Vertices: *9 {{ a: 0,0,0,1,0,0,0,1,0 }}
        PolygonVertexIndex: *3 {{ a: 0,1,-3 }}
    }}
    Model: 3, "Model::Thing_LOD{0}", "Mesh" {{
        Shading: T
        Culling: "CullingOff"
    }}
    Model: 4, "Model::Thing_LOD{1}", "Mesh" {{
        Shading: T
        Culling: "CullingOff"
    }}
    Model: 6, "Model::Thing_LOD{2}", "Mesh" {{
        Shading: T
        Culling: "CullingOff"
    }}
}}
Connections:  {{
    C: "OO",1,3
    C: "OO",2,4
    C: "OO",5,6
    C: "OO",3,0
    C: "OO",4,0
    C: "OO",6,0
}}
)",
        nameCase.suffixes_[0], nameCase.suffixes_[1], nameCase.suffixes_[2]);

    struct LodCallback : public GLTFImporterCallback
    {
        ea::vector<ea::vector<float>> lodDistances_;

        void OnModelLoaded(ModelView& modelView) override
        {
            for (const GeometryView& geometry : modelView.GetGeometries())
            {
                ea::vector<float>& distances = lodDistances_.emplace_back();
                for (const GeometryLODView& lod : geometry.lods_)
                    distances.push_back(lod.lodDistance_);
            }
        }
    };

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    LodCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Lod/", &callback));

    REQUIRE(callback.lodDistances_.size() == 1);
    const ea::vector<float>& distances = callback.lodDistances_[0];
    REQUIRE(distances.size() == 3);
    if (nameCase.indices_)
    {
        // The plain numbers 1 and 2 are too small: the camera is always farther away than 1 or 2 model sizes.
        CHECK(distances[0] == 0.0f);
        CHECK(distances[1] > 2.0f);
        CHECK(distances[2] > distances[1]);
        CHECK(distances[2] == Catch::Approx(distances[1] * 2.0f));
    }
    else
    {
        CHECK(distances[0] == Catch::Approx(ToFloat(nameCase.suffixes_[0])));
        CHECK(distances[1] == Catch::Approx(ToFloat(nameCase.suffixes_[1])));
        CHECK(distances[2] == Catch::Approx(ToFloat(nameCase.suffixes_[2])));
    }
}

TEST_CASE("FBX importer uses the NURBS surface step with a limit")
{
    const unsigned step = GENERATE(2u, 1000u);
    const unsigned segments = ea::min(step, 16u);

    // A bilinear patch with one span in each direction.
    const ea::string fbx = Format(R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {{
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}}
Objects:  {{
    Geometry: 1, "Geometry::Patch", "NurbsSurface" {{
        Type: "NurbsSurface"
        NurbsSurfaceVersion: 100
        NurbsSurfaceOrder: 2,2
        Dimensions: 2,2
        Step: {0},{0}
        Form: "Open", "Open"
        Points: *16 {{ a: 0,0,0,1,1,0,0,1,0,0,1,1,1,0,1,1 }}
        KnotVectorU: *4 {{ a: 0,0,1,1 }}
        KnotVectorV: *4 {{ a: 0,0,1,1 }}
    }}
    Model: 2, "Model::Patch", "NurbsSurface" {{
        Shading: T
        Culling: "CullingOff"
    }}
}}
Connections:  {{
    C: "OO",1,2
    C: "OO",2,0
}}
)",
        step);

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Nurbs/", &callback));

    CHECK(callback.numIndices_ == segments * segments * 2 * 3);
}

TEST_CASE("FBX importer rejects NURBS surfaces with overflowing dimensions")
{
    // 2^32 * 2^32 wraps to zero in 64 bits, so an empty control point array matches the dimensions.
    // ufbx_evaluate_nurbs_surface() reads out of the array if the surface loads.
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Patch", "NurbsSurface" {
        Type: "NurbsSurface"
        NurbsSurfaceVersion: 100
        NurbsSurfaceOrder: 2,2
        Dimensions: 4294967296,4294967296
        Step: 1,1
        Form: "Open", "Open"
        Points: *0 { a: }
        KnotVectorU: *4 { a: 0,0,1,1 }
        KnotVectorV: *4 { a: 0,0,1,1 }
    }
    Model: 2, "Model::Patch", "NurbsSurface" {
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FBXImporter importer{context, GetTestSettings()};
    if (importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}))
        CHECK(importer.Process("", "NurbsOverflow/", nullptr));
}

TEST_CASE("FBX importer skips NURBS curves with an invalid basis")
{
    // The knot vector is too short for the order, so ufbx marks the basis as invalid.
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Curve", "NurbsCurve" {
        Type: "NurbsCurve"
        NurbsCurveVersion: 100
        Order: 4
        Dimension: 3
        Form: "Open"
        Rational: 0
        Points: *8 { a: 0,0,0,1,1,0,0,1 }
        KnotVector: *2 { a: 0,1 }
    }
    Model: 2, "Model::Curve", "NurbsCurve" {
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    CHECK(importer.Process("", "NurbsInvalid/", nullptr));
}

TEST_CASE("FBX importer resolves texture paths relative to a document with empty path segments")
{
    // Each empty segment of the document path adds "..\" to the relative path. ufbx 0.23.0 allocated
    // too small a buffer for this case: an assert in Debug and a heap overflow in Release.
    const ea::string separators(200, '\\');
    const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
    SceneInfo: "SceneInfo::GlobalInfo", "UserData" {
        Type: "UserData"
        Version: 100
        Properties70:  {
            P: "DocumentUrl", "KString", "Url", "", "D:\Dev\data)"
        + separators + R"(model.fbx"
        }
    }
}
Objects:  {
    Texture: 4, "Texture::Inner", "" {
        Type: "TextureVideoClip"
        FileName: "D:/Dev/directory/inner.png"
    }
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FBXImporter importer{context, GetTestSettings()};
    CHECK(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
}

TEST_CASE("FBX importer replaces non-finite vertex data with zero")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: -1.#NAN,-0.#INF,1.#INF,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
        LayerElementNormal: 0 {
            MappingInformationType: "ByPolygonVertex"
            ReferenceInformationType: "Direct"
            Normals: *9 { a: 1.#NAN,0,1,0,0,1,0,0,1 }
        }
        LayerElementUV: 0 {
            MappingInformationType: "ByPolygonVertex"
            ReferenceInformationType: "Direct"
            UV: *6 { a: 1.#INF,0,1,0,0,1 }
        }
        Layer: 0 {
            LayerElement: { Type: "LayerElementNormal" TypedIndex: 0 }
            LayerElement: { Type: "LayerElementUV" TypedIndex: 0 }
        }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",2,0
}
)";

    struct FiniteCallback : public GLTFImporterCallback
    {
        unsigned numVertices_{};
        bool allFinite_{true};
        BoundingBox boundingBox_;

        void OnModelLoaded(ModelView& modelView) override
        {
            const auto isFinite = [](const Vector4& value)
            {
                return std::isfinite(value.x_) && std::isfinite(value.y_) && std::isfinite(value.z_)
                    && std::isfinite(value.w_);
            };
            for (const GeometryView& geometry : modelView.GetGeometries())
            {
                for (const GeometryLODView& lod : geometry.lods_)
                {
                    for (const ModelVertex& vertex : lod.vertices_)
                    {
                        ++numVertices_;
                        allFinite_ &= isFinite(vertex.position_) && isFinite(vertex.normal_)
                            && isFinite(vertex.tangent_) && isFinite(vertex.uv_[0]);
                        boundingBox_.Merge(vertex.GetPosition());
                    }
                }
            }
        }
    };

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FiniteCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "NonFinite/", &callback));

    CHECK(callback.numVertices_ == 3);
    CHECK(callback.allFinite_);
    CHECK(std::isfinite(callback.boundingBox_.min_.x_));
    CHECK(std::isfinite(callback.boundingBox_.max_.z_));
}

TEST_CASE("FBX importer splits oversized bone palettes")
{
    constexpr unsigned NumWeightedBones = 258;
    const ea::string fbx = CreateOversizedSkinFBX(NumWeightedBones);

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Palette/", nullptr));

    auto cache = context->GetSubsystem<ResourceCache>();
    Model* model = cache->GetExistingResource<Model>("Palette/Models/Oversized.mdl");
    REQUIRE(model);
    REQUIRE(model->GetGeometryBoneMappings().size() == model->GetNumGeometries());
    CHECK(model->GetNumGeometries() > 1);

    ea::unordered_set<unsigned> mappedBones;
    unsigned numIndices = 0;
    for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
    {
        const ea::vector<unsigned>& mapping = model->GetGeometryBoneMappings()[i];
        CHECK(mapping.size() <= Graphics::GetMaxBones());
        mappedBones.insert(mapping.begin(), mapping.end());
        REQUIRE(model->GetGeometry(i, 0));
        numIndices += model->GetGeometry(i, 0)->GetIndexCount();
    }
    CHECK(mappedBones.size() == NumWeightedBones);
    CHECK(numIndices == NumWeightedBones);
}

TEST_CASE("FBX importer recalculates incomplete tangent bases")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 {
            a: 0,0,0,1,0,0,0,1,0
        }
        PolygonVertexIndex: *3 {
            a: 0,1,-3
        }
        LayerElementTangent: 0 {
            MappingInformationType: "ByPolygonVertex"
            ReferenceInformationType: "Direct"
            Tangents: *9 {
                a: 0,1,0,0,1,0,0,1,0
            }
        }
        LayerElementUV: 0 {
            MappingInformationType: "ByPolygonVertex"
            ReferenceInformationType: "Direct"
            UV: *6 {
                a: 0,0,1,0,0,1
            }
        }
        Layer: 0 {
            LayerElement:  {
                Type: "LayerElementTangent"
                TypedIndex: 0
            }
            LayerElement:  {
                Type: "LayerElementUV"
                TypedIndex: 0
            }
        }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Tangent/", &callback));

    CHECK(Abs(callback.tangent_.x_) > 0.9f);
    CHECK(Abs(callback.tangent_.y_) < 0.1f);
}

TEST_CASE("FBX importer preserves point and line faces")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Mixed", "Mesh" {
        Vertices: *12 { a: 0,0,0,1,0,0,1,1,0,0,1,0 }
        PolygonVertexIndex: *6 { a: -1,1,-3,0,2,-4 }
    }
    Model: 2, "Model::Mixed", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Mixed/", &callback));

    CHECK(callback.primitiveTypes_ == ea::vector<PrimitiveType>{TRIANGLE_LIST, LINE_LIST, POINT_LIST});
}

TEST_CASE("FBX importer applies per-vertex blend shape weights")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
    Creator: "Blender-- 4.0.0"
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Geometry: 2, "Geometry::Weighted", "Shape" {
        Indexes: *3 { a: 0,1,2 }
        Vertices: *9 { a: 0,1,0,0,1,0,0,1,0 }
        Normals: *9 { a: 0,0,1,0,0,1,0,0,1 }
    }
    Model: 3, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Deformer: 4, "Deformer::Blend", "BlendShape" {
        Version: 100
    }
    Deformer: 5, "Deformer::Weighted", "BlendShapeChannel" {
        Version: 100
        DeformPercent: 0
        FullWeights: *3 { a: 0,50,100 }
    }
}
Connections:  {
    C: "OO",2,5
    C: "OO",5,4
    C: "OO",4,1
    C: "OO",1,3
    C: "OO",3,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Weighted/", &callback));

    REQUIRE(callback.vertexMorphs_.size() == 2);
    CHECK(callback.vertexMorphs_[0].index_ == 1);
    CHECK(callback.vertexMorphs_[0].positionDelta_ == Vector3{0.0f, 0.5f, 0.0f});
    CHECK(callback.vertexMorphs_[0].normalDelta_ == Vector3{0.0f, 0.0f, 0.5f});
    CHECK(callback.vertexMorphs_[1].index_ == 2);
    CHECK(callback.vertexMorphs_[1].positionDelta_ == Vector3::UP);
    CHECK(callback.vertexMorphs_[1].normalDelta_ == Vector3::FORWARD);
}

TEST_CASE("FBX importer attaches one component for combined LODs")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::High", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Geometry: 2, "Geometry::Low", "Mesh" {
        Vertices: *9 { a: 0,0,0,2,0,0,0,2,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    NodeAttribute: 9, "NodeAttribute::LOD Group", "LodGroup" {
        Properties70:  {
            P: "Thresholds|Level0", "double", "Number", "",10
            P: "ThresholdsUsedAsPercentage", "bool", "", "",0
        }
    }
    Model: 10, "Model::LOD Group", "Null" {
    }
    Model: 11, "Model::High", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Model: 12, "Model::Low", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,11
    C: "OO",2,12
    C: "OO",9,10
    C: "OO",11,10
    C: "OO",12,10
    C: "OO",10,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "LOD/", nullptr));

    auto cache = context->GetSubsystem<ResourceCache>();
    PrefabResource* prefab = cache->GetExistingResource<PrefabResource>("LOD/Prefab.prefab");
    REQUIRE(prefab);
    auto scene = MakeShared<Scene>(context);
    Node* root = scene->InstantiatePrefab(prefab);
    REQUIRE(root);
    ea::vector<StaticModel*> models;
    root->FindComponents<StaticModel>(models);
    REQUIRE(models.size() == 1);
    REQUIRE(models[0]->GetModel());
    CHECK(models[0]->GetModel()->GetNumGeometryLodLevels(0) == 2);
}

TEST_CASE("FBX importer gives independent skeletons separate hosts")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::First", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Geometry: 2, "Geometry::Second", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 11, "Model::First", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Model: 12, "Model::Second", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,11
    C: "OO",2,12
    C: "OO",11,0
    C: "OO",12,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    callback.artificialSkinNodes_ = {"First", "Second"};
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Skeletons/", &callback));

    auto cache = context->GetSubsystem<ResourceCache>();
    PrefabResource* prefab = cache->GetExistingResource<PrefabResource>("Skeletons/Prefab.prefab");
    REQUIRE(prefab);
    auto scene = MakeShared<Scene>(context);
    Node* root = scene->InstantiatePrefab(prefab);
    REQUIRE(root);
    ea::vector<AnimatedModel*> models;
    root->FindComponents<AnimatedModel>(models);
    REQUIRE(models.size() == 2);
    CHECK(models[0]->GetNode() != models[1]->GetNode());
    CHECK(models[0]->IsMaster());
    CHECK(models[1]->IsMaster());
}

TEST_CASE("FBX importer releases cached resources after processing failure")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Material: 3, "Material::Failure Material", "" {
        Version: 102
        ShadingModel: "lambert"
        Properties70:  {
            P: "DiffuseColor", "Color", "", "A",1,1,1
        }
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",3,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto cache = context->GetSubsystem<ResourceCache>();
    CaptureImporterCallback callback;
    callback.failOnModel_ = true;
    callback.resourceCache_ = cache;
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    CHECK_FALSE(importer.Process("", "Failure/", &callback));

    REQUIRE(callback.materialWasCached_);
    REQUIRE_FALSE(callback.materialName_.empty());
    CHECK(cache->GetExistingResource<Material>(callback.materialName_) == nullptr);
}

TEST_CASE("FBX importer prepares resources on a worker thread")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Material: 3, "Material::Async", "" {
        Version: 102
        ShadingModel: "lambert"
        Properties70:  {
            P: "DiffuseColor", "Color", "", "A",0.25,0.5,0.75
        }
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",3,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FBXImporter importer{context, GetTestSettings()};
    bool prepared = false;
    std::thread worker([&]
    {
        prepared = importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()})
            && importer.BeginProcess("", "Async/", nullptr);
    });
    worker.join();
    REQUIRE(prepared);
    REQUIRE(importer.EndProcess());

    auto cache = context->GetSubsystem<ResourceCache>();
    Material* material = cache->GetExistingResource<Material>("Async/Materials/Async_Lit.xml");
    REQUIRE(material);
    CHECK(material->GetTechnique(0));
    PrefabResource* prefab = cache->GetExistingResource<PrefabResource>("Async/Prefab.prefab");
    REQUIRE(prefab);
    auto scene = MakeShared<Scene>(context);
    Node* root = scene->InstantiatePrefab(prefab);
    REQUIRE(root);
    StaticModel* model = root->FindComponent<StaticModel>();
    REQUIRE(model);
    CHECK(model->GetMaterial() == material);
}

TEST_CASE("FBX importer keeps missing texture search inside its root")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Material: 3, "Material::Normal", "" {
        Version: 102
        ShadingModel: "phong"
    }
    Texture: 4, "Texture::Missing", "" {
        Type: "TextureVideoClip"
        FileName: "rbfx-escaped-normal.png"
        RelativeFilename: "rbfx-escaped-normal.png"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",3,2
    C: "OP",4,3,"NormalMap"
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fileSystem = context->GetSubsystem<FileSystem>();
    TemporaryDir sourceDir{context, fileSystem->GetTemporaryDir() + "rbfx-fbx-texture-search-tests"};
    TemporaryDir escapedDir{context, fileSystem->GetTemporaryDir() + "rbfx-fbx-texture-escape-tests"};
    REQUIRE(fileSystem->CreateDirs(sourceDir.GetPath(), "project/scenes"));

    Image escapedImage{context};
    REQUIRE(escapedImage.SetSize(1, 1, 4));
    const unsigned char escapedPixel[] = {128, 128, 255, 255};
    escapedImage.SetData(escapedPixel);
    REQUIRE(escapedImage.SavePNG(escapedDir.GetPath() + "rbfx-escaped-normal.png"));

    const ea::string fbxFileName = sourceDir.GetPath() + "project/scenes/model.fbx";
    File file{context, fbxFileName, FILE_WRITE};
    REQUIRE(file.Write(fbx.data(), fbx.size()) == fbx.size());
    file.Close();

    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFile(fbxFileName));
    REQUIRE(importer.Process("", "Normal/", nullptr));

    auto cache = context->GetSubsystem<ResourceCache>();
    CHECK(cache->GetExistingResource<Material>("Normal/Materials/Normal_Lit.xml"));
    CHECK_FALSE(cache->GetExistingResource<Material>("Normal/Materials/Normal_LitNormalMap.xml"));
    PrefabResource* prefab = cache->GetExistingResource<PrefabResource>("Normal/Prefab.prefab");
    REQUIRE(prefab);
    auto scene = MakeShared<Scene>(context);
    Node* root = scene->InstantiatePrefab(prefab);
    REQUIRE(root);
    StaticModel* model = root->FindComponent<StaticModel>();
    REQUIRE(model);
    CHECK(model->GetMaterial() == cache->GetExistingResource<Material>("Normal/Materials/Normal_Lit.xml"));
}

TEST_CASE("FBX importer keeps the texture fallback search order and folder budget")
{
    // The fallback search sorts the folders by name and searches only the first 512 of them.
    const ea::string textureFolder = GENERATE(ea::string{"d001"}, ea::string{"d599"});
    const bool expectFound = textureFolder == "d001";

    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
        LayerElementUV: 0 {
            MappingInformationType: "ByPolygonVertex"
            ReferenceInformationType: "Direct"
            UV: *6 { a: 0,0,1,0,0,1 }
        }
        Layer: 0 {
            LayerElement: { Type: "LayerElementUV" TypedIndex: 0 }
        }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Material: 3, "Material::Normal", "" {
        Version: 102
        ShadingModel: "phong"
    }
    Texture: 4, "Texture::Budget", "" {
        Type: "TextureVideoClip"
        FileName: "rbfx-budget-normal.png"
        RelativeFilename: "rbfx-budget-normal.png"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",3,2
    C: "OP",4,3,"NormalMap"
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fileSystem = context->GetSubsystem<FileSystem>();
    TemporaryDir rootDir{context, fileSystem->GetTemporaryDir() + "rbfx-fbx-texture-budget-tests"};
    REQUIRE(fileSystem->CreateDirs(rootDir.GetPath(), "project/scenes"));
    // The fallback search starts two folders above the model folder. Fill it with more folders than the budget.
    for (unsigned i = 0; i < 600; ++i)
        REQUIRE(fileSystem->CreateDir(rootDir.GetPath() + Format("d{:03}", i)));

    Image image{context};
    REQUIRE(image.SetSize(1, 1, 4));
    const unsigned char pixel[] = {128, 128, 255, 255};
    image.SetData(pixel);
    REQUIRE(image.SavePNG(rootDir.GetPath() + textureFolder + "/rbfx-budget-normal.png"));

    const ea::string fbxFileName = rootDir.GetPath() + "project/scenes/model.fbx";
    File file{context, fbxFileName, FILE_WRITE};
    REQUIRE(file.Write(fbx.data(), fbx.size()) == fbx.size());
    file.Close();

    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFile(fbxFileName));
    REQUIRE(importer.Process("", "Budget/", nullptr));

    auto cache = context->GetSubsystem<ResourceCache>();
    CHECK(static_cast<bool>(cache->GetExistingResource<Material>("Budget/Materials/Normal_LitNormalMap.xml"))
        == expectFound);
    CHECK(static_cast<bool>(cache->GetExistingResource<Material>("Budget/Materials/Normal_Lit.xml")) != expectFound);
}

TEST_CASE("FBX importer preserves hidden mesh visibility")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Hidden", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 2, "Model::Hidden", "Mesh" {
        Properties70:  {
            P: "Visibility", "Visibility", "", "A",0
        }
        Shading: T
        Culling: "CullingOff"
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    callback.artificialSkinNodes_ = {"Hidden"};
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "Visibility/", &callback));

    auto cache = context->GetSubsystem<ResourceCache>();
    PrefabResource* prefab = cache->GetExistingResource<PrefabResource>("Visibility/Prefab.prefab");
    REQUIRE(prefab);
    auto scene = MakeShared<Scene>(context);
    Node* root = scene->InstantiatePrefab(prefab);
    REQUIRE(root);
    Node* hidden = root->FindChild("Hidden", true);
    REQUIRE(hidden);
    AnimatedModel* model = root->FindComponent<AnimatedModel>();
    REQUIRE(model);
    CHECK_FALSE(hidden->IsEnabled());
    CHECK_FALSE(model->IsEnabled());
    CHECK_FALSE(model->IsEnabledEffective());
}

TEST_CASE("FBX importer resolves morph paths after creating skeleton hosts")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Morph", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Geometry: 2, "Geometry::Raised", "Shape" {
        Indexes: *1 { a: 0 }
        Vertices: *3 { a: 0,1,0 }
    }
    Model: 3, "Model::Morph", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Deformer: 4, "Deformer::Blend", "BlendShape" {
        Version: 100
    }
    Deformer: 5, "Deformer::Raised", "BlendShapeChannel" {
        Version: 100
        DeformPercent: 0
        FullWeights: *1 { a: 100 }
    }
    Geometry: 6, "Geometry::Skin", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 7, "Model::Skin", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    AnimationStack: 10, "AnimStack::Morph", "" {
        Properties70:  {
            P: "LocalStart", "KTime", "Time", "",0
            P: "LocalStop", "KTime", "Time", "",46186158000
        }
    }
    AnimationLayer: 11, "AnimLayer::Base", "" {
    }
    AnimationCurveNode: 12, "AnimCurveNode::DeformPercent", "" {
        Properties70:  {
            P: "d|DeformPercent", "Number", "", "A",0
        }
    }
    AnimationCurve: 13, "AnimCurve::DeformPercent", "" {
        Default: 0
        KeyVer: 4008
        KeyTime: *2 { a: 0,46186158000 }
        KeyValueFloat: *2 { a: 0,100 }
        KeyAttrFlags: *2 { a: 24836,24836 }
        KeyAttrDataFloat: *8 { a: 0,0,0,0,0,0,0,0 }
        KeyAttrRefCount: *2 { a: 1,1 }
    }
}
Connections:  {
    C: "OO",2,5
    C: "OO",5,4
    C: "OO",4,1
    C: "OO",1,3
    C: "OO",6,7
    C: "OO",3,7
    C: "OO",7,0
    C: "OO",11,10
    C: "OO",12,11
    C: "OP",12,5,"DeformPercent"
    C: "OP",13,12,"d|DeformPercent"
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    CaptureImporterCallback callback;
    callback.artificialSkinNodes_ = {"Skin"};
    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()}));
    REQUIRE(importer.Process("", "MorphPath/", &callback));

    const auto track = ea::find_if(callback.variantTrackNames_.begin(), callback.variantTrackNames_.end(),
        [](const ea::string& name) { return name.ends_with("/Morphs/0"); });
    REQUIRE(track != callback.variantTrackNames_.end());
    CHECK(track->contains("Skin Skeleton/Skin/Morph"));
}

TEST_CASE("FBX importer normalizes merged animation space")
{
    static const ea::string primaryFBX = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
GlobalSettings:  {
    Version: 1000
    Properties70:  {
        P: "UpAxis", "int", "Integer", "",1
        P: "UpAxisSign", "int", "Integer", "",1
        P: "FrontAxis", "int", "Integer", "",2
        P: "FrontAxisSign", "int", "Integer", "",-1
        P: "CoordAxis", "int", "Integer", "",0
        P: "CoordAxisSign", "int", "Integer", "",1
        P: "UnitScaleFactor", "double", "Number", "",1
        P: "OriginalUnitScaleFactor", "double", "Number", "",1
    }
}
Objects:  {
    Model: 1, "Model::Animated", "Null" {
    }
}
Connections:  {
    C: "OO",1,0
}
)";
    static const ea::string secondaryFBX = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
GlobalSettings:  {
    Version: 1000
    Properties70:  {
        P: "UpAxis", "int", "Integer", "",2
        P: "UpAxisSign", "int", "Integer", "",1
        P: "FrontAxis", "int", "Integer", "",1
        P: "FrontAxisSign", "int", "Integer", "",-1
        P: "CoordAxis", "int", "Integer", "",0
        P: "CoordAxisSign", "int", "Integer", "",1
        P: "UnitScaleFactor", "double", "Number", "",100
        P: "OriginalUnitScaleFactor", "double", "Number", "",100
    }
}
Objects:  {
    Model: 1, "Model::Animated", "Null" {
    }
    AnimationStack: 10, "AnimStack::Move", "" {
        Properties70:  {
            P: "LocalStart", "KTime", "Time", "",0
            P: "LocalStop", "KTime", "Time", "",46186158000
        }
    }
    AnimationLayer: 11, "AnimLayer::Base", "" {
    }
    AnimationCurveNode: 12, "AnimCurveNode::T", "" {
        Properties70:  {
            P: "d|X", "Number", "", "A",0
            P: "d|Y", "Number", "", "A",0
            P: "d|Z", "Number", "", "A",0
        }
    }
    AnimationCurve: 13, "AnimCurve::Z", "" {
        Default: 0
        KeyVer: 4008
        KeyTime: *2 { a: 0,46186158000 }
        KeyValueFloat: *2 { a: 0,1 }
        KeyAttrFlags: *2 { a: 24836,24836 }
        KeyAttrDataFloat: *8 { a: 0,0,0,0,0,0,0,0 }
        KeyAttrRefCount: *2 { a: 1,1 }
    }
}
Connections:  {
    C: "OO",1,0
    C: "OO",11,10
    C: "OO",12,11
    C: "OP",12,1,"Lcl Translation"
    C: "OP",13,12,"d|Z"
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fileSystem = context->GetSubsystem<FileSystem>();
    TemporaryDir temporaryDir{context, fileSystem->GetTemporaryDir() + "rbfx-fbx-merge-units-tests"};
    const ea::string primaryFileName = temporaryDir.GetPath() + "primary.fbx";
    const ea::string secondaryFileName = temporaryDir.GetPath() + "secondary.fbx";
    File primaryFile{context, primaryFileName, FILE_WRITE};
    REQUIRE(primaryFile.Write(primaryFBX.data(), primaryFBX.size()) == primaryFBX.size());
    primaryFile.Close();
    File secondaryFile{context, secondaryFileName, FILE_WRITE};
    REQUIRE(secondaryFile.Write(secondaryFBX.data(), secondaryFBX.size()) == secondaryFBX.size());
    secondaryFile.Close();

    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFile(primaryFileName));
    REQUIRE(importer.MergeFile(secondaryFileName, "Move"));
    CHECK_FALSE(importer.IsAnimationOnly(0));
    CHECK(importer.IsAnimationOnly(1));
    CHECK(importer.GetNumAnimations(0) == 0);
    CHECK(importer.GetNumAnimations(1) == 1);
    REQUIRE(importer.Process("", "MergeUnits/", nullptr));

    auto cache = context->GetSubsystem<ResourceCache>();
    Animation* animation = cache->GetExistingResource<Animation>("MergeUnits/Animations/Move.ani");
    REQUIRE(animation);
    const auto positionTrack = ea::find_if(animation->GetVariantTracks().begin(), animation->GetVariantTracks().end(),
        [](const auto& entry) { return entry.second.name_.ends_with("/@/Position"); });
    REQUIRE(positionTrack != animation->GetVariantTracks().end());
    REQUIRE(positionTrack->second.keyFrames_.size() == 2);

    PrefabResource* prefab = cache->GetExistingResource<PrefabResource>("MergeUnits/Prefab.prefab");
    REQUIRE(prefab);
    auto scene = MakeShared<Scene>(context);
    Node* root = scene->InstantiatePrefab(prefab);
    REQUIRE(root);
    Node* animated = root->FindChild("Animated", true);
    REQUIRE(animated);
    animated->SetPosition(positionTrack->second.keyFrames_.back().value_.GetVector3());
    CHECK(animated->GetWorldPosition().y_ == Catch::Approx(1.0f).margin(M_EPSILON));
}

TEST_CASE("FBX importer converts glossiness textures to roughness")
{
    static const ea::string fbx = R"(; FBX 7.7.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1004
    FBXVersion: 7700
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
        LayerElementUV: 0 {
            Name: "UVChannel_1"
            MappingInformationType: "ByPolygonVertex"
            ReferenceInformationType: "Direct"
            UV: *6 { a: 0,0,1,0,0,1 }
        }
        Layer: 0 {
            LayerElement:  {
                Type: "LayerElementUV"
                TypedIndex: 0
            }
        }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Material: 3, "Material::Gloss", "" {
        Version: 102
        ShadingModel: "unknown"
        Properties70:  {
            P: "3dsMax|ClassIDa", "int", "Integer", "",-804315648
            P: "3dsMax|ClassIDb", "int", "Integer", "",31173939
            P: "3dsMax|SuperClassID", "int", "Integer", "",3072
            P: "3dsMax|main|baseColor", "ColorAndAlpha", "", "A",1,1,1,1
            P: "3dsMax|main|glossiness", "Float", "", "A",0.25
            P: "3dsMax|main|glossiness_map", "Reference", "", "A"
            P: "3dsMax|main|useGlossiness", "Integer", "", "A",1
        }
    }
    Video: 4, "Video::Gloss", "Clip" {
        Type: "Clip"
        Filename: "gloss.png"
        RelativeFilename: "gloss.png"
    }
    Texture: 5, "Texture::Gloss", "" {
        Type: "TextureVideoClip"
        Version: 202
        TextureName: "Texture::Gloss"
        Properties70:  {
            P: "UVSet", "KString", "", "", "UVChannel_1"
        }
        Media: "Video::Gloss"
        FileName: "gloss.png"
        RelativeFilename: "gloss.png"
        ModelUVTranslation: 0,0
        ModelUVScaling: 1,1
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",3,2
    C: "OP",5,3,"3dsMax|main|glossiness_map"
    C: "OO",4,5
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fileSystem = context->GetSubsystem<FileSystem>();
    TemporaryDir temporaryDir{context, fileSystem->GetTemporaryDir() + "rbfx-fbx-gloss-tests"};
    const ea::string rootPath = temporaryDir.GetPath();

    Image sourceImage{context};
    REQUIRE(sourceImage.SetSize(1, 1, 4));
    const unsigned char sourcePixel[] = {64, 64, 64, 255};
    sourceImage.SetData(sourcePixel);
    REQUIRE(sourceImage.SavePNG(rootPath + "gloss.png"));

    const ea::string fbxFileName = rootPath + "gloss.fbx";
    File file{context, fbxFileName, FILE_WRITE};
    REQUIRE(file.Write(fbx.data(), fbx.size()) == fbx.size());
    file.Close();

    REQUIRE(fileSystem->CreateDirs(rootPath, "Textures"));
    REQUIRE(fileSystem->CreateDirs(rootPath, "Materials"));
    REQUIRE(fileSystem->CreateDirs(rootPath, "Models"));

    FBXImporter importer{context, GetTestSettings()};
    REQUIRE(importer.LoadFile(fbxFileName));
    REQUIRE(importer.Process(rootPath, "Gloss/", nullptr));

    auto cache = context->GetSubsystem<ResourceCache>();
    Material* material = cache->GetExistingResource<Material>("Gloss/Materials/Gloss_Lit.xml");
    REQUIRE(material);
    CHECK(material->GetShaderParameter(ShaderConsts::Material_Roughness).GetFloat()
        == Catch::Approx(1.0f).margin(M_EPSILON));

    REQUIRE(importer.SaveResources());
    const auto& resources = importer.GetSavedResources();
    const auto properties = resources.find("Gloss/Textures/Properties.png");
    REQUIRE(properties != resources.end());
    Image packedImage{context};
    REQUIRE(packedImage.LoadFile(properties->second));
    CHECK((packedImage.GetPixelInt(0, 0) & 0xffu) == 239u);
}

TEST_CASE("FBX importer keeps default material state on a worker thread")
{
    static const ea::string fbx = R"(; FBX 7.4.0 project file
FBXHeaderExtension:  {
    FBXHeaderVersion: 1003
    FBXVersion: 7400
}
Objects:  {
    Geometry: 1, "Geometry::Triangle", "Mesh" {
        Vertices: *9 { a: 0,0,0,1,0,0,0,1,0 }
        PolygonVertexIndex: *3 { a: 0,1,-3 }
    }
    Model: 2, "Model::Triangle", "Mesh" {
        Shading: T
        Culling: "CullingOff"
    }
    Material: 3, "Material::Single", "" {
        Version: 102
        ShadingModel: "lambert"
        Properties70:  {
            P: "DiffuseColor", "Color", "", "A",0.25,0.5,0.75
        }
    }
}
Connections:  {
    C: "OO",1,2
    C: "OO",3,2
    C: "OO",2,0
}
)";

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    FBXImporter importer{context, GetTestSettings()};
    bool prepared = false;
    std::thread worker([&]
    {
        prepared = importer.LoadFileBinary({reinterpret_cast<const unsigned char*>(fbx.data()), fbx.size()})
            && importer.BeginProcess("", "DefaultState/", nullptr);
    });
    worker.join();
    REQUIRE(prepared);
    REQUIRE(importer.EndProcess());

    auto cache = context->GetSubsystem<ResourceCache>();
    Material* material = cache->GetExistingResource<Material>("DefaultState/Materials/Single_Lit.xml");
    REQUIRE(material);
    CHECK(material->GetCullMode() == CULL_CCW);
    CHECK(material->GetRenderOrder() == 128);
    CHECK(material->GetOcclusion());
}
