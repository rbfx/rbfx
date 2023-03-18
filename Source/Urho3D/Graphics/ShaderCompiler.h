#pragma once
#include "../Graphics/GraphicsDefs.h"
#include "../Graphics/ShaderDefineArray.h"
#include "../Graphics/ShaderVariation.h"

namespace Urho3D
{
    enum class ShaderLanguage {
        GLSL=0,
        HLSL
    };
    struct ShaderCompilerDesc {
        ea::string name_;
        ea::string code_;
        ea::string entryPoint_;
        ShaderType type_;
        ShaderDefineArray defines_;
        ShaderLanguage language_;
    };
    class URHO3D_API ShaderCompiler {
    public:
        ShaderCompiler(ShaderCompilerDesc desc);
        bool Compile();
        const ea::vector<uint8_t>& GetByteCode() const { return byteCode_; }
        const ea::string& GetCompilerOutput() const { return compilerOutput_; }
        bool IsUsedCBufferSlot(ShaderParameterGroup grp) { return constantBufferSlots_[grp]; }
        bool IsUsedTextureSlot(TextureUnit unit) { return textureSlots_[unit]; }
        const ea::vector<VertexElement> GetVertexElements() const { return vertexElements_; }
        const ea::unordered_map<StringHash, ShaderParameter> GetShaderParams() const { return parameters_; }
    private:
#ifdef WIN32
        bool CompileHLSL();
        bool ReflectHLSL(unsigned char* byteCode, size_t byteCodeSize);
#endif
#ifdef URHO3D_SPIRV
        bool CompileGLSL();
#endif
#ifdef URHO3D_DILIGENT
        void RemapInputLayout(ea::string& sourceCode);
#endif
        ShaderCompilerDesc desc_;

        ea::vector<uint8_t> byteCode_;
        ea::string compilerOutput_;

        ea::vector<VertexElement> vertexElements_;
        ea::array<bool, MAX_TEXTURE_UNITS> textureSlots_;
        ea::array<bool, MAX_SHADER_PARAMETER_GROUPS> constantBufferSlots_;
        ea::unordered_map<StringHash, ShaderParameter> parameters_;

#ifdef URHO3D_DILIGENT
        /// This is used for remapping the whole semantic attributes to attribn diligent like.
        ea::vector<ea::pair<unsigned, VertexElementSemantic>> inputLayoutMapping_;
#endif
    };
}
