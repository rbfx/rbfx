// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/LightProbeGroup.h"
#include "Urho3D/Math/Matrix3.h"
#include "Urho3D/Math/Sphere.h"
#include "Urho3D/Math/TetrahedralMesh.h"
#include "Urho3D/Math/Vector3.h"
#include "Urho3D/Resource/BinaryFile.h"
#include "Urho3D/Scene/Component.h"

namespace Urho3D
{

/// Global illumination manager.
class URHO3D_API GlobalIllumination : public Component
{
    URHO3D_OBJECT(GlobalIllumination, Component);

public:
    /// Construct.
    explicit GlobalIllumination(Context* context);
    /// Destruct.
    ~GlobalIllumination() override;
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);
    /// Visualize the component as debug geometry.
    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;
    /// Handle scene set for event subscription.
    void OnSceneSet(Scene* previousScene, Scene* scene) override;

    /// Reset light probes.
    void ResetLightProbes();
    /// Compile all enabled light probe groups in the scene.
    void CompileLightProbes();

    /// Sample ambient spherical harmonics.
    SphericalHarmonicsDot9 SampleAmbientSH(const Vector3& position, unsigned& hint) const;
    /// Sample average ambient lighting.
    Vector3 SampleAverageAmbient(const Vector3& position, unsigned& hint) const;

    /// Set emission brightness.
    void SetEmissionBrightness(float emissionBrightness) { emissionBrightness_ = emissionBrightness; }
    /// Return emission brightness.
    float GetEmissionBrightness() const { return emissionBrightness_; }

    /// Set reference on file with baked data.
    void SetFileRef(const ResourceRef& fileRef);
    /// Return reference on file with baked data.
    ResourceRef GetFileRef() const;

    /// Set offset.
    void SetOffset(const Vector3& offset) { offset_ = offset; }
    /// Return offset.
    const Vector3& GetOffset() const { return offset_; }

    /// Serialize GI data. May throw ArchiveException.
    void SerializeData(Archive& archive);

private:
    /// Handle world origin post-update.
    void HandleWorldOriginPostUpdate(VariantMap& eventData);
    /// Reload GI data.
    void ReloadData();

    /// Emission indirect brightness.
    float emissionBrightness_{ 1.0f };

    /// Reference on file with GI data.
    ResourceRef fileRef_{ BinaryFile::GetTypeStatic() };

    /// Offset.
    Vector3 offset_;

    /// Light probes mesh.
    TetrahedralMesh lightProbesMesh_;
    /// Baked light probes data.
    LightProbeCollectionBakedData lightProbesBakedData_;
};

}
