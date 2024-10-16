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

#include <set>
#include <vector>
#include <sstream>
#include <iomanip>
#include "../../GraphicsEngine/interface/Shader.h"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

class ShaderMacroHelper
{
public:
    template <typename DefinitionType>
    ShaderMacroHelper& AddShaderMacro(const Char* Name, DefinitionType Definition)
    {
#if DILIGENT_DEBUG
        for (size_t i = 0; i < m_Macros.size() && m_Macros[i].Definition != nullptr; ++i)
        {
            if (strcmp(m_Macros[i].Name, Name) == 0)
            {
                UNEXPECTED("Macro '", Name, "' already exists. Use Update() to update the macro value.");
            }
        }
#endif
        std::ostringstream ss;
        ss << Definition;
        return AddShaderMacro<const Char*>(Name, ss.str().c_str());
    }

    template <typename DefinitionType>
    ShaderMacroHelper& Add(const Char* Name, DefinitionType Definition)
    {
        return AddShaderMacro(Name, Definition);
    }

    ShaderMacroHelper() = default;

    // NB: we need to make sure that string pointers in m_Macros don't become invalid
    //     after the copy or move due to short string optimization in std::string
    ShaderMacroHelper(const ShaderMacroHelper& rhs) :
        m_Macros{rhs.m_Macros}
    {
        for (auto& Macros : m_Macros)
        {
            if (Macros.Definition != nullptr)
                Macros.Definition = m_StringPool.insert(Macros.Definition).first->c_str();
            if (Macros.Name != nullptr)
                Macros.Name = m_StringPool.insert(Macros.Name).first->c_str();
        }
    }

    ShaderMacroHelper& operator=(const ShaderMacroHelper& rhs)
    {
        *this = ShaderMacroHelper{rhs};
        return *this;
    }

    ShaderMacroHelper(ShaderMacroHelper&&) = default;
    ShaderMacroHelper& operator=(ShaderMacroHelper&&) = default;

    void Clear()
    {
        m_Macros.clear();
        m_StringPool.clear();
    }

    operator ShaderMacroArray() const
    {
        return {
            !m_Macros.empty() ? m_Macros.data() : nullptr,
            static_cast<Uint32>(m_Macros.size()),
        };
    }

    ShaderMacroHelper& RemoveMacro(const Char* Name)
    {
        size_t i = 0;
        while (i < m_Macros.size() && m_Macros[i].Definition != nullptr)
        {
            if (strcmp(m_Macros[i].Name, Name) == 0)
            {
                m_Macros.erase(m_Macros.begin() + i);
                break;
            }
            else
            {
                ++i;
            }
        }

        return *this;
    }

    ShaderMacroHelper& Remove(const Char* Name)
    {
        return RemoveMacro(Name);
    }

    template <typename DefinitionType>
    ShaderMacroHelper& UpdateMacro(const Char* Name, DefinitionType Definition)
    {
        RemoveMacro(Name);
        return AddShaderMacro(Name, Definition);
    }

    template <typename DefinitionType>
    ShaderMacroHelper& Update(const Char* Name, DefinitionType Definition)
    {
        return UpdateMacro(Name, Definition);
    }

    ShaderMacroHelper& Add(const ShaderMacro& Macro)
    {
        const auto* PooledDefinition = m_StringPool.insert(Macro.Definition).first->c_str();
        const auto* PooledName       = m_StringPool.insert(Macro.Name).first->c_str();
        m_Macros.emplace_back(PooledName, PooledDefinition);
        return *this;
    }

    ShaderMacroHelper& operator+=(const ShaderMacroHelper& Macros)
    {
        for (const auto& Macro : Macros.m_Macros)
            Add(Macro);

        return *this;
    }

    ShaderMacroHelper& operator+=(const ShaderMacro& Macro)
    {
        return Add(Macro);
    }

    ShaderMacroHelper& operator+=(const ShaderMacroArray& Macros)
    {
        for (Uint32 i = 0; i < Macros.Count; ++i)
            Add(Macros[i]);

        return *this;
    }

    template <typename T>
    ShaderMacroHelper operator+(const T& Macros) const
    {
        ShaderMacroHelper NewMacros{*this};
        NewMacros += Macros;
        return NewMacros;
    }

    const char* Find(const char* Name) const
    {
        if (Name == nullptr)
            return nullptr;

        for (const auto& Macro : m_Macros)
        {
            if (strcmp(Macro.Name, Name) == 0)
                return Macro.Definition != nullptr ? Macro.Definition : "";
        }
        return nullptr;
    }

private:
    std::vector<ShaderMacro> m_Macros;
    std::set<std::string>    m_StringPool;
};

template <>
inline ShaderMacroHelper& ShaderMacroHelper::AddShaderMacro(const Char* Name, const Char* Definition)
{
    return Add(ShaderMacro{Name, Definition != nullptr ? Definition : ""});
}

template <>
inline ShaderMacroHelper& ShaderMacroHelper::AddShaderMacro(const Char* Name, bool Definition)
{
    return AddShaderMacro<const Char*>(Name, Definition ? "1" : "0");
}

template <>
inline ShaderMacroHelper& ShaderMacroHelper::AddShaderMacro(const Char* Name, float Definition)
{
    std::ostringstream ss;

    // Make sure that when floating point represents integer, it is still
    // written as float: 1024.0, but not 1024. This is essential to
    // avoid type conversion issues in GLES.
    if (Definition == static_cast<float>(static_cast<int>(Definition)))
        ss << std::fixed << std::setprecision(1);

    ss << Definition;
    return AddShaderMacro<const Char*>(Name, ss.str().c_str());
}

template <>
inline ShaderMacroHelper& ShaderMacroHelper::AddShaderMacro(const Char* Name, Uint32 Definition)
{
    // Make sure that uint constants have the 'u' suffix to avoid problems in GLES.
    std::ostringstream ss;
    ss << Definition << 'u';
    return AddShaderMacro<const Char*>(Name, ss.str().c_str());
}

template <>
inline ShaderMacroHelper& ShaderMacroHelper::AddShaderMacro(const Char* Name, Uint16 Definition)
{
    return AddShaderMacro(Name, Uint32{Definition});
}

template <>
inline ShaderMacroHelper& ShaderMacroHelper::AddShaderMacro(const Char* Name, Uint8 Definition)
{
    return AddShaderMacro(Name, Uint32{Definition});
}

template <>
inline ShaderMacroHelper& ShaderMacroHelper::AddShaderMacro(const Char* Name, Int8 Definition)
{
    return AddShaderMacro(Name, Int32{Definition});
}

#define ADD_SHADER_MACRO_ENUM_VALUE(Helper, EnumValue) Helper.AddShaderMacro(#EnumValue, static_cast<int>(EnumValue));

} // namespace Diligent
