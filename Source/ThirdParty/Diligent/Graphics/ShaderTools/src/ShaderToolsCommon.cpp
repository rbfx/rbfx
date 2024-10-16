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

#include "ShaderToolsCommon.hpp"

#include <unordered_set>

#include "BasicFileSystem.hpp"
#include "DebugUtilities.hpp"
#include "DataBlobImpl.hpp"
#include "StringDataBlobImpl.hpp"
#include "GraphicsAccessories.hpp"
#include "ParsingTools.hpp"

namespace Diligent
{

using namespace Parsing;

namespace
{

constexpr ShaderMacro VSMacros[]  = {{"VERTEX_SHADER", "1"}};
constexpr ShaderMacro PSMacros[]  = {{"FRAGMENT_SHADER", "1"}, {"PIXEL_SHADER", "1"}};
constexpr ShaderMacro GSMacros[]  = {{"GEOMETRY_SHADER", "1"}};
constexpr ShaderMacro HSMacros[]  = {{"TESS_CONTROL_SHADER", "1"}, {"HULL_SHADER", "1"}};
constexpr ShaderMacro DSMacros[]  = {{"TESS_EVALUATION_SHADER", "1"}, {"DOMAIN_SHADER", "1"}};
constexpr ShaderMacro CSMacros[]  = {{"COMPUTE_SHADER", "1"}};
constexpr ShaderMacro ASMacros[]  = {{"TASK_SHADER", "1"}, {"AMPLIFICATION_SHADER", "1"}};
constexpr ShaderMacro MSMacros[]  = {{"MESH_SHADER", "1"}};
constexpr ShaderMacro RGMacros[]  = {{"RAY_GEN_SHADER", "1"}};
constexpr ShaderMacro RMMacros[]  = {{"RAY_MISS_SHADER", "1"}};
constexpr ShaderMacro RCHMacros[] = {{"RAY_CLOSEST_HIT_SHADER", "1"}};
constexpr ShaderMacro RAHMacros[] = {{"RAY_ANY_HIT_SHADER", "1"}};
constexpr ShaderMacro RIMacros[]  = {{"RAY_INTERSECTION_SHADER", "1"}};
constexpr ShaderMacro RCMacros[]  = {{"RAY_CALLABLE_SHADER", "1"}};

#define SHADER_MACROS_ARRAY(Macros) \
    ShaderMacroArray { Macros, _countof(Macros) }

} // namespace

ShaderMacroArray GetShaderTypeMacros(SHADER_TYPE Type)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    switch (Type)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:           return SHADER_MACROS_ARRAY(VSMacros);
        case SHADER_TYPE_PIXEL:            return SHADER_MACROS_ARRAY(PSMacros);
        case SHADER_TYPE_GEOMETRY:         return SHADER_MACROS_ARRAY(GSMacros);
        case SHADER_TYPE_HULL:             return SHADER_MACROS_ARRAY(HSMacros);
        case SHADER_TYPE_DOMAIN:           return SHADER_MACROS_ARRAY(DSMacros);
        case SHADER_TYPE_COMPUTE:          return SHADER_MACROS_ARRAY(CSMacros);
        case SHADER_TYPE_AMPLIFICATION:    return SHADER_MACROS_ARRAY(ASMacros);
        case SHADER_TYPE_MESH:             return SHADER_MACROS_ARRAY(MSMacros);
        case SHADER_TYPE_RAY_GEN:          return SHADER_MACROS_ARRAY(RGMacros);
        case SHADER_TYPE_RAY_MISS:         return SHADER_MACROS_ARRAY(RMMacros);
        case SHADER_TYPE_RAY_CLOSEST_HIT:  return SHADER_MACROS_ARRAY(RCHMacros);
        case SHADER_TYPE_RAY_ANY_HIT:      return SHADER_MACROS_ARRAY(RAHMacros);
        case SHADER_TYPE_RAY_INTERSECTION: return SHADER_MACROS_ARRAY(RIMacros);
        case SHADER_TYPE_CALLABLE:         return SHADER_MACROS_ARRAY(RCMacros);
        // clang-format on
        case SHADER_TYPE_TILE:
            UNEXPECTED("Unsupported shader type");
            return ShaderMacroArray{};
        default:
            UNEXPECTED("Unexpected shader type");
            return ShaderMacroArray{};
    }
}

void AppendShaderMacros(std::string& Source, const ShaderMacroArray& Macros)
{
    if (!Macros)
        return;

    for (size_t i = 0; i < Macros.Count; ++i)
    {
        Source += "#define ";
        Source += Macros[i].Name;
        Source += ' ';
        Source += Macros[i].Definition;
        Source += "\n";
    }
}

void AppendShaderTypeDefinitions(std::string& Source, SHADER_TYPE Type)
{
    AppendShaderMacros(Source, GetShaderTypeMacros(Type));
}

void AppendPlatformDefinition(std::string& Source)
{
#if PLATFORM_WIN32
    Source.append("#define PLATFORM_WIN32 1\n");
#elif PLATFORM_UNIVERSAL_WINDOWS
    Source.append("#define PLATFORM_UWP 1\n");
#elif PLATFORM_LINUX
    Source.append("#define PLATFORM_LINUX 1\n");
#elif PLATFORM_MACOS
    Source.append("#define PLATFORM_MACOS 1\n");
#elif PLATFORM_IOS
    Source.append("#define PLATFORM_IOS 1\n");
#elif PLATFORM_TVOS
    Source.append("#define PLATFORM_TVOS 1\n");
#elif PLATFORM_ANDROID
    Source.append("#define PLATFORM_ANDROID 1\n");
#elif PLATFORM_EMSCRIPTEN
    Source.append("#define PLATFORM_EMSCRIPTEN 1\n");
#else
#    error Unexpected platform
#endif
}

static const std::string ShaderSourceLanguageKey = "$SHADER_SOURCE_LANGUAGE";

void AppendShaderSourceLanguageDefinition(std::string& Source, SHADER_SOURCE_LANGUAGE Language)
{
    Source += "/*";
    Source += ShaderSourceLanguageKey;
    Source += '=';
    Source += std::to_string(static_cast<Uint32>(Language));
    Source += "*/";
}

SHADER_SOURCE_LANGUAGE ParseShaderSourceLanguageDefinition(const std::string& Source)
{
    //  /*$SHADER_SOURCE_LANGUAGE=1*/
    //                               ^
    auto it = Source.end();
    if (it == Source.begin())
        return SHADER_SOURCE_LANGUAGE_DEFAULT;

    --it;
    //  /*$SHADER_SOURCE_LANGUAGE=1*/
    //                              ^
    if (it == Source.begin() || *it != '/')
        return SHADER_SOURCE_LANGUAGE_DEFAULT;

    --it;
    //  /*$SHADER_SOURCE_LANGUAGE=1*/
    //                             ^
    if (it == Source.begin() || *it != '*')
        return SHADER_SOURCE_LANGUAGE_DEFAULT;

    --it;
    while (true)
    {
        if (it == Source.begin())
            return SHADER_SOURCE_LANGUAGE_DEFAULT;

        if (*it == '*')
        {
            //  /*$SHADER_SOURCE_LANGUAGE=1*/
            //   ^
            --it;
            if (*it == '/')
                break;
        }
        else
        {
            --it;
        }
    }

    //  /*$SHADER_SOURCE_LANGUAGE=1*/
    //  ^
    const auto KeyPos = Source.find(ShaderSourceLanguageKey, it - Source.begin() + 2);
    if (KeyPos == std::string::npos)
        return SHADER_SOURCE_LANGUAGE_DEFAULT;

    it = Source.begin() + KeyPos + ShaderSourceLanguageKey.length();
    while (it != Source.end() && *it == ' ')
        ++it;

    //  /*$SHADER_SOURCE_LANGUAGE=1*/
    //                           ^
    if (it == Source.end() || *it != '=')
        return SHADER_SOURCE_LANGUAGE_DEFAULT;

    ++it;
    while (it != Source.end() && *it == ' ')
        ++it;

    if (it == Source.end() || !IsNum(*it))
        return SHADER_SOURCE_LANGUAGE_DEFAULT;

    Uint32 Lang = 0;
    for (; it != Source.end() && IsNum(*it); ++it)
        Lang = Lang * 10 + (*it - '0');

    return Lang >= 0 && Lang < SHADER_SOURCE_LANGUAGE_COUNT ?
        static_cast<SHADER_SOURCE_LANGUAGE>(Lang) :
        SHADER_SOURCE_LANGUAGE_DEFAULT;
}

ShaderSourceFileData ReadShaderSourceFile(const char*                      SourceCode,
                                          size_t                           SourceLength,
                                          IShaderSourceInputStreamFactory* pShaderSourceStreamFactory,
                                          const char*                      FilePath) noexcept(false)
{
    ShaderSourceFileData SourceData;
    if (SourceCode != nullptr)
    {
        VERIFY(FilePath == nullptr, "FilePath must be null when SourceCode is not null");
        SourceData.Source       = SourceCode;
        SourceData.SourceLength = StaticCast<Uint32>(SourceLength != 0 ? SourceLength : strlen(SourceCode));
    }
    else
    {
        if (pShaderSourceStreamFactory != nullptr)
        {
            if (FilePath != nullptr)
            {
                RefCntAutoPtr<IFileStream> pSourceStream;
                pShaderSourceStreamFactory->CreateInputStream(FilePath, &pSourceStream);
                if (pSourceStream == nullptr)
                    LOG_ERROR_AND_THROW("Failed to load shader source file '", FilePath, '\'');

                SourceData.pFileData = DataBlobImpl::Create();
                pSourceStream->ReadBlob(SourceData.pFileData);
                SourceData.Source       = reinterpret_cast<char*>(SourceData.pFileData->GetDataPtr());
                SourceData.SourceLength = StaticCast<Uint32>(SourceData.pFileData->GetSize());
            }
            else
            {
                UNEXPECTED("FilePath is null");
            }
        }
        else
        {
            UNEXPECTED("Input stream factory is null");
        }
    }

    return SourceData;
}

void AppendLine1Marker(std::string& Source, const char* FileName)
{
    Source.append("#line 1");
    if (FileName != nullptr)
    {
        Source.append(" \"");
        Source.append(FileName);
        Source.append("\"");
    }
    Source.append("\n");
}

void AppendShaderSourceCode(std::string& Source, const ShaderCreateInfo& ShaderCI) noexcept(false)
{
    VERIFY_EXPR(ShaderCI.ByteCode == nullptr);
    const auto SourceData = ReadShaderSourceFile(ShaderCI);
    Source.append(SourceData.Source, SourceData.SourceLength);
}

static String ParserErrorMessage(const char* Message, const Char* pBuffer, const Char* pBufferEnd, const char* pCurrPos)
{
    size_t      Line       = 0;
    const auto* pLineStart = pBuffer;
    for (const auto* c = pBuffer; c < pCurrPos; ++c)
    {
        if (*c == '\n')
        {
            ++Line;
            pLineStart = c;
        }
    }
    size_t LineOffset = pCurrPos - pLineStart;

    std::stringstream Stream;
    Stream << "[" << Line << "," << LineOffset << "]: " << Message << std::endl
           << GetContext(pBuffer, pBufferEnd, pCurrPos, 1);
    return Stream.str();
}


// https://github.com/tomtom-international/cpp-dependencies/blob/a91f330e97c6b9e4e9ecd81f43c4a40e044d4bbc/src/Input.cpp
template <typename HandlerType, typename ErrorHandlerType>
bool FindIncludes(const char* pBuffer, size_t BufferSize, HandlerType&& IncludeHandler, ErrorHandlerType ErrorHandler)
{
    if (BufferSize == 0)
        return true;

    const auto*       pCurrPos   = pBuffer;
    const auto* const pBufferEnd = pBuffer + BufferSize;

    using ErrorType = std::pair<const char*, const char*>;
    try
    {
        while (pCurrPos < pBufferEnd)
        {
            pCurrPos = SkipDelimitersAndComments(pCurrPos, pBufferEnd); // May throw
            if (pCurrPos == pBufferEnd)
                return true;

            if (*pCurrPos != '#')
            {
                ++pCurrPos;
                continue;
            }

            const auto pIncludeStart = pCurrPos;
            // # /* ... */ include <File.h>
            // ^

            auto pLineEnd = SkipLine(pCurrPos, pBufferEnd);

            pCurrPos = SkipDelimitersAndComments(pIncludeStart + 1, pBufferEnd, " \t", SKIP_COMMENT_FLAG_MULTILINE); // May throw
            if (pCurrPos == pBufferEnd)
                return true;

            if (pCurrPos >= pLineEnd)
                continue;

            // # /* ... */ include <File.h>
            //             ^

            static constexpr auto IncludeStrLen                 = 7;
            static constexpr char IncludeStr[IncludeStrLen + 1] = "include";

            if (strncmp(pCurrPos, IncludeStr, IncludeStrLen) != 0)
            {
                // #define MACRO
                //  ^
                pCurrPos = pLineEnd;
                continue;
            }

            pCurrPos += IncludeStrLen;


            // # /* ... */ include <File.h>
            //                    ^
            auto pOpenQuoteOrAngleBracket = SkipDelimitersAndComments(pCurrPos, pBufferEnd, " \t", SKIP_COMMENT_FLAG_MULTILINE); // May throw
            if (pOpenQuoteOrAngleBracket == pBufferEnd)
                throw ErrorType{pCurrPos, "Unexpected end of file."};

            if (pOpenQuoteOrAngleBracket >= pLineEnd)
                throw ErrorType{pLineEnd, "New line in the include directive."};

            pCurrPos = pOpenQuoteOrAngleBracket;
            // # /* ... */ include <File.h>
            //                     ^

            if (pCurrPos < pBufferEnd && *pCurrPos != '<' && *pCurrPos != '"')
                throw ErrorType{pCurrPos, "\'<\' or \'\"\' is expected"};

            auto ClosingChar = *pCurrPos == '<' ? '>' : '"';
            ++pCurrPos;
            while (pCurrPos < pBufferEnd && *pCurrPos != ClosingChar)
                ++pCurrPos;

            if (pCurrPos == pBufferEnd)
                throw ErrorType{pOpenQuoteOrAngleBracket, (ClosingChar == '>' ? "Unable to find the matching angle bracket" : "Unable to find the matching closing quote")};

            if (pCurrPos >= pLineEnd)
                throw ErrorType{pLineEnd, "New line in the file name."};

            IncludeHandler(std::string{pOpenQuoteOrAngleBracket + 1, pCurrPos}, pIncludeStart - pBuffer, pCurrPos - pBuffer + 1);

            ++pCurrPos;
        }
    }
    catch (const std::pair<const char*, const char*>& err)
    {
        const auto* pos = err.first;
        const auto* msg = err.second;
        ErrorHandler(ParserErrorMessage(msg, pBuffer, pBufferEnd, pos));
        return false;
    }
    catch (...)
    {
        ErrorHandler("Unknown error");
        return false;
    }

    return true;
}

static void ProcessIncludeErrorHandler(const ShaderCreateInfo& ShaderCI, const std::string& Error) noexcept(false)
{
    std::string FileInfo;
    if (ShaderCI.FilePath != nullptr)
    {
        FileInfo = "file '";
        FileInfo += ShaderCI.FilePath;
        FileInfo += '\'';
    }
    else if (ShaderCI.Desc.Name != nullptr)
    {
        FileInfo = "shader '";
        FileInfo += ShaderCI.Desc.Name;
        FileInfo += '\'';
    }
    throw std::pair<std::string, std::string>{std::move(FileInfo), Error};
}

template <typename IncludeHandlerType>
void ProcessShaderIncludesImpl(const ShaderCreateInfo& ShaderCI, std::unordered_set<std::string>& Includes, IncludeHandlerType&& IncludeHandler) noexcept(false)
{
    const auto SourceData = ReadShaderSourceFile(ShaderCI);

    ShaderIncludePreprocessInfo FileInfo;
    FileInfo.Source       = SourceData.Source;
    FileInfo.SourceLength = SourceData.SourceLength;
    FileInfo.FilePath     = ShaderCI.FilePath != nullptr ? ShaderCI.FilePath : "";

    FindIncludes(
        FileInfo.Source, FileInfo.SourceLength,
        [&](const std::string& FilePath, size_t Start, size_t End) //
        {
            if (!Includes.insert(FilePath).second)
                return;

            auto IncludeCI{ShaderCI};
            IncludeCI.FilePath     = FilePath.c_str();
            IncludeCI.Source       = nullptr;
            IncludeCI.SourceLength = 0;
            ProcessShaderIncludesImpl(IncludeCI, Includes, IncludeHandler);
        },
        std::bind(ProcessIncludeErrorHandler, ShaderCI, std::placeholders::_1));

    if (IncludeHandler)
        IncludeHandler(FileInfo);
}

bool ProcessShaderIncludes(const ShaderCreateInfo& ShaderCI, std::function<void(const ShaderIncludePreprocessInfo&)> IncludeHandler) noexcept
{
    try
    {
        std::unordered_set<std::string> Includes;
        ProcessShaderIncludesImpl(ShaderCI, Includes, IncludeHandler);
        return true;
    }
    catch (const std::pair<std::string, std::string>& ErrInfo)
    {
        LOG_ERROR_MESSAGE("Failed to process includes in ", ErrInfo.first, ": ", ErrInfo.second);
        return false;
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to process includes in shader '", (ShaderCI.Desc.Name != nullptr ? ShaderCI.Desc.Name : ""), "'.");
        return false;
    }
}

static std::string UnrollShaderIncludesImpl(ShaderCreateInfo ShaderCI, std::unordered_set<std::string>& AllIncludes) noexcept(false)
{
    const auto SourceData = ReadShaderSourceFile(ShaderCI);

    ShaderCI.Source       = SourceData.Source;
    ShaderCI.SourceLength = SourceData.SourceLength;
    ShaderCI.FilePath     = nullptr;

    std::stringstream Stream;
    size_t            PrevIncludeEnd = 0;

    FindIncludes(
        ShaderCI.Source, ShaderCI.SourceLength, [&](const std::string& Path, size_t IncludeStart, size_t IncludeEnd) {
            // Insert text before the include start
            Stream.write(ShaderCI.Source + PrevIncludeEnd, IncludeStart - PrevIncludeEnd);

            if (AllIncludes.insert(Path).second)
            {
                // Process the #include directive
                ShaderCreateInfo IncludeCI{ShaderCI};
                IncludeCI.Source       = nullptr;
                IncludeCI.SourceLength = 0;
                IncludeCI.FilePath     = Path.c_str();
                auto UnrolledInclude   = UnrollShaderIncludesImpl(IncludeCI, AllIncludes);
                Stream << UnrolledInclude;
            }

            PrevIncludeEnd = IncludeEnd;
        },
        std::bind(ProcessIncludeErrorHandler, ShaderCI, std::placeholders::_1));

    // Insert text after the last include
    Stream.write(ShaderCI.Source + PrevIncludeEnd, ShaderCI.SourceLength - PrevIncludeEnd);

    return Stream.str();
}

std::string UnrollShaderIncludes(const ShaderCreateInfo& ShaderCI) noexcept(false)
{
    std::unordered_set<std::string> Includes;
    if (ShaderCI.FilePath != nullptr)
        Includes.emplace(ShaderCI.FilePath);

    try
    {
        return UnrollShaderIncludesImpl(ShaderCI, Includes);
    }
    catch (const std::pair<std::string, std::string>& ErrInfo)
    {
        LOG_ERROR_AND_THROW("Failed to unroll includes in ", ErrInfo.first, ": ", ErrInfo.second);
        return "";
    }
    // Let other exceptions (e.g. 'Failed to load shader source file...') pass through
}

std::string GetShaderCodeTypeName(SHADER_CODE_BASIC_TYPE     BasicType,
                                  SHADER_CODE_VARIABLE_CLASS Class,
                                  Uint32                     NumRows,
                                  Uint32                     NumCols,
                                  SHADER_SOURCE_LANGUAGE     Lang)
{
    if (Class == SHADER_CODE_VARIABLE_CLASS_STRUCT)
        return "struct";

    std::string BasicTypeStr = GetShaderCodeBasicTypeString(BasicType);

    std::string Suffix;
    if (Class == SHADER_CODE_VARIABLE_CLASS_VECTOR)
    {
        Uint32 Dim = 0;
        if (Lang == SHADER_SOURCE_LANGUAGE_GLSL ||
            Lang == SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM)
        {
            Dim = NumRows;
            switch (BasicType)
            {
                // clang-format off
                case SHADER_CODE_BASIC_TYPE_FLOAT: BasicTypeStr ="vec"; break;
                case SHADER_CODE_BASIC_TYPE_INT:   BasicTypeStr ="ivec";break;
                case SHADER_CODE_BASIC_TYPE_UINT:  BasicTypeStr ="uvec";break;
                case SHADER_CODE_BASIC_TYPE_BOOL:  BasicTypeStr ="bvec";break;
                // clang-format on
                default:
                    UNEXPECTED("Unexpected vector basic type");
            }
        }
        else
        {
            Dim = NumCols;
        }

        Suffix = std::to_string(Dim);
    }
    else if (Class == SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS ||
             Class == SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS)
    {
        if (Lang == SHADER_SOURCE_LANGUAGE_GLSL ||
            Lang == SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM)
        {
            BasicTypeStr = "mat";
            Suffix       = std::to_string(NumCols) + "x" + std::to_string(NumRows);
        }
        else
        {
            Suffix = std::to_string(NumRows) + "x" + std::to_string(NumCols);
        }
    }

    return BasicTypeStr + Suffix;
}

} // namespace Diligent
