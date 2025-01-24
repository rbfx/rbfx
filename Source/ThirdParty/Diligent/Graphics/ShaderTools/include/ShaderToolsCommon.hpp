/*
 *  Copyright 2019-2024 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include <functional>
#include <string>
#include <memory>

#include "GraphicsTypes.h"
#include "Shader.h"
#include "RefCntAutoPtr.hpp"
#include "DataBlob.h"
#include "FixedLinearAllocator.hpp"
#include "STDAllocator.hpp"

namespace Diligent
{

/// Returns shader type definition macro(s), e.g., for a vertex shader:
///
///     ShaderMacroArray{{{"VERTEX_SHADER", "1"}}, 1}
///
/// or, for a fragment shader:
///
///     ShaderMacroArray{{{"FRAGMENT_SHADER", "1"}, {"PIXEL_SHADER", "1"}}, 2}
///
/// etc.
ShaderMacroArray GetShaderTypeMacros(SHADER_TYPE Type);


/// Appends shader macro definitions to the end of the source string:
///
///     #define Name[0] Definition[0]
///     #define Name[1] Definition[1]
///     ...
void AppendShaderMacros(std::string& Source, const ShaderMacroArray& Macros);


/// Appends the shader type definition macro(s), e.g., for a vertex shader:
///
///     #define VERTEX_SHADER 1
///
/// or, for a fragment shader:
///
///     #define FRAGMENT_SHADER 1
///     #define PIXEL_SHADER 1
///
/// etc.
void AppendShaderTypeDefinitions(std::string& Source, SHADER_TYPE Type);


/// Appends platform definition macro, e.g. for Windows:
///
///    #define PLATFORM_WIN32 1
///
void AppendPlatformDefinition(std::string& Source);


/// Appends a special comment that contains the shader source language definition.
/// For example, for HLSL:
///
///     /*$SHADER_SOURCE_LANGUAGE=1*/
void AppendShaderSourceLanguageDefinition(std::string& Source, SHADER_SOURCE_LANGUAGE Language);


/// Parses the shader source language definition comment and returns the result.
/// If the comment is not present or can't be parsed, returns SHADER_SOURCE_LANGUAGE_DEFAULT.
SHADER_SOURCE_LANGUAGE ParseShaderSourceLanguageDefinition(const std::string& Source);


struct ShaderSourceFileData
{
    RefCntAutoPtr<IDataBlob> pFileData;
    const char*              Source       = nullptr;
    Uint32                   SourceLength = 0;
};

/// Reads shader source code from a file or uses the one from the shader create info
ShaderSourceFileData ReadShaderSourceFile(const char*                      SourceCode,
                                          size_t                           SourceLength,
                                          IShaderSourceInputStreamFactory* pShaderSourceStreamFactory,
                                          const char*                      FilePath) noexcept(false);

inline ShaderSourceFileData ReadShaderSourceFile(const ShaderCreateInfo& ShaderCI) noexcept(false)
{
    return ReadShaderSourceFile(ShaderCI.Source, ShaderCI.SourceLength, ShaderCI.pShaderSourceStreamFactory, ShaderCI.FilePath);
}

/// Appends #line 1 directive to the source string to make sure that the error messages
/// contain correct line numbers.
void AppendLine1Marker(std::string& Source, const char* FileName);

/// Appends shader source code to the source string
void AppendShaderSourceCode(std::string& Source, const ShaderCreateInfo& ShaderCI) noexcept(false);


/// Shader include preprocess info.
struct ShaderIncludePreprocessInfo
{
    /// The source code of the included file.
    const Char* Source = nullptr;

    /// Length of the included source code
    size_t SourceLength = 0;

    /// The path to the included file.
    std::string FilePath;
};

/// The function recursively finds all include files in the shader and calls the
/// IncludeHandler function for all source files, including the original one.
/// Includes are processed in a depth-first order such that original source file is processed last.
bool ProcessShaderIncludes(const ShaderCreateInfo& ShaderCI, std::function<void(const ShaderIncludePreprocessInfo&)> IncludeHandler) noexcept;

///  Unrolls all include files into a single file
std::string UnrollShaderIncludes(const ShaderCreateInfo& ShaderCI) noexcept(false);

std::string GetShaderCodeTypeName(SHADER_CODE_BASIC_TYPE     BasicType,
                                  SHADER_CODE_VARIABLE_CLASS Class,
                                  Uint32                     NumRows,
                                  Uint32                     NumCols,
                                  SHADER_SOURCE_LANGUAGE     Lang);

struct ShaderCodeVariableDescX : ShaderCodeVariableDesc
{
    ShaderCodeVariableDescX() noexcept {}

    ShaderCodeVariableDescX(const ShaderCodeVariableDesc& Member) noexcept :
        ShaderCodeVariableDesc{Member},
        // clang-format off
        NameCopy    {Member.Name     != nullptr ? Member.Name     : ""},
        TypeNameCopy{Member.TypeName != nullptr ? Member.TypeName : ""}
    // clang-format on
    {
        Name     = NameCopy.c_str();
        TypeName = TypeNameCopy.c_str();

        Members.reserve(Member.NumMembers);
        for (Uint32 i = 0; i < Member.NumMembers; ++i)
            AddMember(Member.pMembers[i]);
    }

    ShaderCodeVariableDescX(const ShaderCodeVariableDescX&) = delete;
    ShaderCodeVariableDescX& operator=(const ShaderCodeVariableDescX&) = delete;

    ShaderCodeVariableDescX(ShaderCodeVariableDescX&& Other) noexcept :
        ShaderCodeVariableDesc{Other},
        // clang-format off
        NameCopy    {std::move(Other.NameCopy)},
        TypeNameCopy{std::move(Other.TypeNameCopy)},
        Members     {std::move(Other.Members)}
    // clang-format on
    {
        Name     = NameCopy.c_str();
        TypeName = TypeNameCopy.c_str();
        pMembers = Members.data();
    }


    ShaderCodeVariableDescX& operator=(ShaderCodeVariableDescX&& RHS) noexcept
    {
        static_cast<ShaderCodeVariableDesc&>(*this) = RHS;

        Members      = std::move(RHS.Members);
        NameCopy     = std::move(RHS.NameCopy);
        TypeNameCopy = std::move(RHS.TypeNameCopy);

        Name     = NameCopy.c_str();
        TypeName = TypeNameCopy.c_str();
        pMembers = Members.data();

        return *this;
    }

    size_t AddMember(const ShaderCodeVariableDesc& Member)
    {
        const auto idx = Members.size();
        Members.emplace_back(Member);
        NumMembers = static_cast<Uint32>(Members.size());
        pMembers   = Members.data();
        return idx;
    }

    size_t AddMember(ShaderCodeVariableDescX&& Member)
    {
        const auto idx = Members.size();
        Members.emplace_back(std::move(Member));
        NumMembers = static_cast<Uint32>(Members.size());
        pMembers   = Members.data();
        return idx;
    }

    void SetName(std::string NewName)
    {
        NameCopy = std::move(NewName);
        Name     = NameCopy.c_str();
    }

    void SetTypeName(std::string NewTypeName)
    {
        TypeNameCopy = std::move(NewTypeName);
        TypeName     = TypeNameCopy.c_str();
    }

    void SetDefaultTypeName(SHADER_SOURCE_LANGUAGE Language)
    {
        SetTypeName(GetShaderCodeTypeName(BasicType, Class, NumRows, NumColumns, Language));
    }

    ShaderCodeVariableDescX& GetMember(size_t idx)
    {
        return Members[idx];
    }

    template <typename HandlerType>
    void ProcessMembers(HandlerType&& Handler)
    {
        Handler(Members);
        pMembers   = Members.data();
        NumMembers = StaticCast<Uint32>(Members.size());
    }

    ShaderCodeVariableDescX* FindMember(const char* MemberName)
    {
        VERIFY_EXPR(MemberName != nullptr);
        for (auto& Member : Members)
        {
            if (strcmp(Member.Name, MemberName) == 0)
                return &Member;
        }
        return nullptr;
    }

    static void ReserveSpaceForMembers(FixedLinearAllocator& Allocator, const std::vector<ShaderCodeVariableDescX>& Members)
    {
        if (Members.empty())
            return;

        Allocator.AddSpace<ShaderCodeVariableDesc>(Members.size());
        for (const auto& Member : Members)
        {
            Allocator.AddSpaceForString(Member.Name);
            Allocator.AddSpaceForString(Member.TypeName);
        }

        for (const auto& Member : Members)
            ReserveSpaceForMembers(Allocator, Member.Members);
    }

    static ShaderCodeVariableDesc* CopyMembers(FixedLinearAllocator& Allocator, const std::vector<ShaderCodeVariableDescX>& Members)
    {
        if (Members.empty())
            return nullptr;

        auto* pMembers = Allocator.ConstructArray<ShaderCodeVariableDesc>(Members.size());
        for (size_t i = 0; i < Members.size(); ++i)
        {
            pMembers[i]          = Members[i];
            pMembers[i].Name     = Allocator.CopyString(Members[i].Name);
            pMembers[i].TypeName = Allocator.CopyString(Members[i].TypeName);
        }

        for (size_t i = 0; i < Members.size(); ++i)
            pMembers[i].pMembers = CopyMembers(Allocator, Members[i].Members);

        return pMembers;
    }

private:
    std::string NameCopy;
    std::string TypeNameCopy;

    std::vector<ShaderCodeVariableDescX> Members;
    // When adding new members, don't forget to update move ctor/assignment
};

struct ShaderCodeBufferDescX : ShaderCodeBufferDesc
{
    ShaderCodeBufferDescX() noexcept
    {}

    ShaderCodeBufferDescX(const ShaderCodeBufferDesc& Desc) noexcept :
        ShaderCodeBufferDesc{Desc}
    {}

    ShaderCodeBufferDescX(const ShaderCodeBufferDescX&) = delete;
    ShaderCodeBufferDescX(ShaderCodeBufferDescX&&)      = default;

    size_t AddVariable(const ShaderCodeVariableDesc& Var)
    {
        const auto idx = Variables.size();
        Variables.emplace_back(Var);
        NumVariables = static_cast<Uint32>(Variables.size());
        pVariables   = Variables.data();
        return idx;
    }

    size_t AddVariable(ShaderCodeVariableDescX&& Var)
    {
        const auto idx = Variables.size();
        Variables.emplace_back(std::move(Var));
        NumVariables = static_cast<Uint32>(Variables.size());
        pVariables   = Variables.data();
        return idx;
    }

    ShaderCodeVariableDescX& GetVariable(size_t idx)
    {
        return Variables[idx];
    }

    void AssignVariables(std::vector<ShaderCodeVariableDescX>&& _Variables)
    {
        Variables    = std::move(_Variables);
        pVariables   = Variables.data();
        NumVariables = static_cast<Uint32>(Variables.size());
    }

    void ReserveSpace(FixedLinearAllocator& Allocator) const
    {
        ShaderCodeVariableDescX::ReserveSpaceForMembers(Allocator, Variables);
    }

    ShaderCodeBufferDesc MakeCopy(FixedLinearAllocator& Allocator) const
    {
        ShaderCodeBufferDesc Desc{*this};
        Desc.pVariables = ShaderCodeVariableDescX::CopyMembers(Allocator, Variables);
        return Desc;
    }

    template <typename IterType>
    static std::unique_ptr<void, STDDeleterRawMem<void>> PackArray(IterType RangeStart, IterType RangeEnd, IMemoryAllocator& RawAllocator)
    {
        FixedLinearAllocator Allocator{RawAllocator};

        const auto Size = RangeEnd - RangeStart;
        Allocator.AddSpace<ShaderCodeBufferDesc>(Size);
        for (auto Refl = RangeStart; Refl != RangeEnd; ++Refl)
            Refl->ReserveSpace(Allocator);

        Allocator.Reserve();
        std::unique_ptr<void, STDDeleterRawMem<void>> DataBuffer{Allocator.ReleaseOwnership(), STDDeleterRawMem<void>{RawAllocator}};

        auto* pRefl = Allocator.ConstructArray<ShaderCodeBufferDesc>(Size);
        VERIFY_EXPR(pRefl == DataBuffer.get());
        for (auto Refl = RangeStart; Refl != RangeEnd; ++Refl)
            *(pRefl++) = Refl->MakeCopy(Allocator);

        return DataBuffer;
    }

private:
    std::vector<ShaderCodeVariableDescX> Variables;
};

} // namespace Diligent
