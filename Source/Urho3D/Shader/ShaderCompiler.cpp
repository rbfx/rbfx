// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Shader/ShaderCompiler.h"

#include "Urho3D/Core/Assert.h"

#if D3D11_SUPPORTED || D3D12_SUPPORTED
    #include <d3dcompiler.h>
#endif

namespace Urho3D
{

namespace
{

const char* GetShaderProfile(ShaderType type)
{
    switch (type)
    {
    case VS: return "vs_5_0";
    case PS: return "ps_5_0";
    case GS: return "gs_5_0";
    case HS: return "hs_5_0";
    case DS: return "ds_5_0";
    case CS: return "cs_5_0";
    default: URHO3D_ASSERT(0); return "";
    }
}

} // namespace

bool CompileHLSLToBinary(ByteVector& bytecode, ea::string& compilerOutput, ea::string_view sourceCode, ShaderType type)
{
    bytecode.clear();
    compilerOutput.clear();

#if D3D11_SUPPORTED || D3D12_SUPPORTED
    const char* profile = GetShaderProfile(type);

    unsigned flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
    if (type == PS)
        flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
    #ifdef URHO3D_DEBUG
    // Debug flag will help developer to inspect shader code.
    flags |= D3DCOMPILE_DEBUG;
    #endif

    ID3DBlob* bytecodeBlob = nullptr;
    ID3DBlob* errorMessagesBlob = nullptr;

    const HRESULT result = D3DCompile(sourceCode.data(), sourceCode.length(), nullptr, nullptr, nullptr, "main",
        profile, flags, 0, &bytecodeBlob, &errorMessagesBlob);

    if (errorMessagesBlob)
    {
        const char* errorMessageBegin = reinterpret_cast<const char*>(errorMessagesBlob->GetBufferPointer());
        compilerOutput.assign(errorMessageBegin, errorMessageBegin + errorMessagesBlob->GetBufferSize());
        errorMessagesBlob->Release();
    }

    if (bytecodeBlob)
    {
        const auto bytecodeSize = static_cast<unsigned>(bytecodeBlob->GetBufferSize());
        bytecode.resize(bytecodeSize);
        memcpy(bytecode.data(), bytecodeBlob->GetBufferPointer(), bytecodeSize);
        bytecodeBlob->Release();
    }

    return SUCCEEDED(result);
#else
    return false;
#endif
}

} // namespace Urho3D
