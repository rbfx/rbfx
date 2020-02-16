#pragma once

#include "../Core/Object.h"
#include "../Graphics/GraphicsDefs.h"

#include <EASTL/map.h>
#include <EASTL/vector.h>

#if defined(URHO3D_D3D11)
    struct ID3D11Buffer;
    struct ID3D11SamplerState;
    struct ID3D11ShaderResourceView;
    struct ID3D11UnorderedAccessView;
#elif defined(URHO3D_OPENGL)
#endif

// The only absolute is that D3D9 doesn't have compute ... barring an OpenCL fallback using cl_khr_dx9_media_sharing.
#ifndef URHO3D_D3D9

namespace Urho3D
{

class ConstantBuffer;
class Graphics;
class ShaderVariation;
class Texture;

/// Common interface for GP-GPU that is responsible for dispatch and keeping track of the compute-specific state of the DX and GL APIs. Usage has no explicit rules but is most likely appropriate in
/// event handlers for E_BEGINRENDERING, E_ENDRENDERING, E_BEGINVIEWUPDATE, E_BEGINVIEWRENDER, and other events that are clean segues.
class URHO3D_API ComputeDevice : public Object
{
    URHO3D_OBJECT(ComputeDevice, Object);
public:
    /// Construct.
    ComputeDevice(Context*, Graphics*);
    /// Destruct.
    virtual ~ComputeDevice();

    /// Returns true if this compute device can actually execute, ie. a D3D9 level target on D3D11.
    bool IsSupported() const;

    /// Set a texture for reading as a traditional texture.
    bool SetReadTexture(Texture* texture, unsigned unit);
    /// Set a constant buffer for standard usage.
    bool SetConstantBuffer(ConstantBuffer* buffer, unsigned unit);

    /// Sets a texture for image write usage.
    bool SetWriteTexture(Texture* texture, unsigned unit, unsigned faceIndex, unsigned mipLevel);

    /// Sets or clears the compute shader to use.
    bool SetProgram(SharedPtr<ShaderVariation> computeShader);
    /// Dispatches the compute call, will queue a barrier as needed.
    void Dispatch(unsigned xDim, unsigned yDim, unsigned zDim);

private:
    /// Apply all dirty GPU object bindings.
    void ApplyBindings();
    /// Removes any constructed resources in response to a GPUObject::Release of a resource
    void HandleGPUResourceRelease(StringHash eventID, VariantMap& eventData);
    /// Frees any locally created GPU objects.
    void ReleaseLocalState();

#if defined(URHO3D_D3D11)
    /// Record for a mip+face UAV combination.
    struct UAVBinding {
        ID3D11UnorderedAccessView* uav_;
        unsigned face_;
        unsigned mipLevel_;
    };

    /// As needed UAVs are constructed for textures and those UAVs are saved here.
    eastl::map<WeakPtr<Object>, eastl::vector<UAVBinding> > constructedUAVs_;
    /// As needed SRVs are constructed for buffers and those are saved here.
    eastl::map<WeakPtr<Object>, ID3D11ShaderResourceView*> constructedBufferSRVs_;

    /// List of sampler bindings.
    ID3D11SamplerState* samplerBindings_[MAX_TEXTURE_UNITS];
    /// List of SRV bindings (textures or buffers).
    ID3D11ShaderResourceView* shaderResourceViews_[MAX_TEXTURE_UNITS];
    /// Constant buffer bindings.
    ID3D11Buffer* constantBuffers_[MAX_TEXTURE_UNITS];
    /// UAV targets for writing.
    ID3D11UnorderedAccessView* uavs_[MAX_TEXTURE_UNITS];
#elif defined(URHO3D_OPENGL)
    
#endif

    /// Handle to the graphics object for device specific access.
    Graphics* graphics_;
    /// Active compute shader that will be invoked with dispatch.
    SharedPtr<ShaderVariation> computeShader_;
    /// Tags samplers as dirty.
    bool samplersDirty_;
    /// Tags constant buffers as dirty.
    bool constantBuffersDirty_;
    /// Tags textures as dirty.
    bool texturesDirty_;
    /// Tags UAVs as dirty.
    bool uavsDirty_;
    /// Tag the shader program as dirty.
    bool programDirty_;
    /// Tag for the availability of compute, determined at startup.
    bool isComputeSupported_;
};
    
}

#endif

