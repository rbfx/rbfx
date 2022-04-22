/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

namespace Diligent
{

namespace
{

const ShaderMacro VSMacros[]  = {{"VERTEX_SHADER", "1"}, {}};
const ShaderMacro PSMacros[]  = {{"FRAGMENT_SHADER", "1"}, {"PIXEL_SHADER", "1"}, {}};
const ShaderMacro GSMacros[]  = {{"GEOMETRY_SHADER", "1"}, {}};
const ShaderMacro HSMacros[]  = {{"TESS_CONTROL_SHADER", "1"}, {"HULL_SHADER", "1"}, {}};
const ShaderMacro DSMacros[]  = {{"TESS_EVALUATION_SHADER", "1"}, {"DOMAIN_SHADER", "1"}, {}};
const ShaderMacro CSMacros[]  = {{"COMPUTE_SHADER", "1"}, {}};
const ShaderMacro ASMacros[]  = {{"TASK_SHADER", "1"}, {"AMPLIFICATION_SHADER", "1"}, {}};
const ShaderMacro MSMacros[]  = {{"MESH_SHADER", "1"}, {}};
const ShaderMacro RGMacros[]  = {{"RAY_GEN_SHADER", "1"}, {}};
const ShaderMacro RMMacros[]  = {{"RAY_MISS_SHADER", "1"}, {}};
const ShaderMacro RCHMacros[] = {{"RAY_CLOSEST_HIT_SHADER", "1"}, {}};
const ShaderMacro RAHMacros[] = {{"RAY_ANY_HIT_SHADER", "1"}, {}};
const ShaderMacro RIMacros[]  = {{"RAY_INTERSECTION_SHADER", "1"}, {}};
const ShaderMacro RCMacros[]  = {{"RAY_CALLABLE_SHADER", "1"}, {}};

} // namespace

const ShaderMacro* GetShaderTypeMacros(SHADER_TYPE Type)
{
    static_assert(SHADER_TYPE_LAST == 0x4000, "Please update the switch below to handle the new shader type");
    switch (Type)
    {
        // clang-format off
        case SHADER_TYPE_VERTEX:           return VSMacros;
        case SHADER_TYPE_PIXEL:            return PSMacros;
        case SHADER_TYPE_GEOMETRY:         return GSMacros;
        case SHADER_TYPE_HULL:             return HSMacros;
        case SHADER_TYPE_DOMAIN:           return DSMacros;
        case SHADER_TYPE_COMPUTE:          return CSMacros;
        case SHADER_TYPE_AMPLIFICATION:    return ASMacros;
        case SHADER_TYPE_MESH:             return MSMacros;
        case SHADER_TYPE_RAY_GEN:          return RGMacros;
        case SHADER_TYPE_RAY_MISS:         return RMMacros;
        case SHADER_TYPE_RAY_CLOSEST_HIT:  return RCHMacros;
        case SHADER_TYPE_RAY_ANY_HIT:      return RAHMacros;
        case SHADER_TYPE_RAY_INTERSECTION: return RIMacros;
        case SHADER_TYPE_CALLABLE:         return RCMacros;
        // clang-format on
        case SHADER_TYPE_TILE:
            UNEXPECTED("Unsupported shader type");
            return nullptr;
        default:
            UNEXPECTED("Unexpected shader type");
            return nullptr;
    }
}

void AppendShaderMacros(std::string& Source, const ShaderMacro* Macros)
{
    if (Macros == nullptr)
        return;

    for (auto* pMacro = Macros; pMacro->Name != nullptr && pMacro->Definition != nullptr; ++pMacro)
    {
        Source += "#define ";
        Source += pMacro->Name;
        Source += ' ';
        Source += pMacro->Definition;
        Source += "\n";
    }
}

void AppendShaderTypeDefinitions(std::string& Source, SHADER_TYPE Type)
{
    AppendShaderMacros(Source, GetShaderTypeMacros(Type));
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

                SourceData.pFileData = MakeNewRCObj<DataBlobImpl>{}(0);
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

void AppendShaderSourceCode(std::string& Source, const ShaderCreateInfo& ShaderCI) noexcept(false)
{
    VERIFY_EXPR(ShaderCI.ByteCode == nullptr);
    const auto SourceData = ReadShaderSourceFile(ShaderCI);
    Source.append(SourceData.Source, SourceData.SourceLength);
}

static String ParserErrorMessage(const char* Message, const Char* pBuffer, const char* pCurrPos)
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
    Stream << "[" << Line << "," << LineOffset << "]: " << Message;
    return Stream.str();
}


// https://github.com/tomtom-international/cpp-dependencies/blob/a91f330e97c6b9e4e9ecd81f43c4a40e044d4bbc/src/Input.cpp
template <typename HandlerType, typename ErrorHandlerType>
bool FindIncludes(const char* pBuffer, size_t BufferSize, HandlerType IncludeHandler, ErrorHandlerType ErrorHandler)
{
    if (BufferSize == 0)
        return true;

    enum State
    {
        None,
        AfterHash,
        AfterInclude,
        InsideIncludeAngleBrackets,
        InsideIncludeQuotes
    } PreprocessorState = None;

    constexpr const Char* MissingEndComment     = "missing end comment.";
    constexpr const Char* MissingOpeningSymbol  = "missing opening quote or angle bracket after the include directive.";
    constexpr const Char* MissingClosingQuote   = "missing closing quote in the include directive.";
    constexpr const Char* MissingClosingBracket = "missing closing angle bracket in the include directive.";

    // Find positions of the first hash and slash
    const Char* NextHash  = static_cast<const char*>(memchr(pBuffer, '#', BufferSize));
    const Char* NextSlash = static_cast<const char*>(memchr(pBuffer, '/', BufferSize));
    size_t      Start     = 0;

    // We iterate over the characters of the buffer
    const auto*       pCurrPos   = pBuffer;
    const auto* const pBufferEnd = pBuffer + BufferSize;
    while (pCurrPos < pBufferEnd)
    {
        switch (PreprocessorState)
        {
            case None:
            {
                // Find a hash character if the current position is greater NextHash
                if (NextHash && NextHash < pCurrPos)
                    NextHash = static_cast<const Char*>(memchr(pCurrPos, '#', pBufferEnd - pCurrPos));

                // Exit from the function if a hash is not found in the buffer
                if (NextHash == nullptr)
                    return true;

                // Find a slash character if the current position is greater NextSlash
                if (NextSlash && NextSlash < pCurrPos)
                    NextSlash = static_cast<const Char*>(memchr(pCurrPos, '/', pBufferEnd - pCurrPos));

                if (NextSlash && NextSlash < NextHash)
                {
                    // Skip all characters if the slash character is before the hash character in the buffer
                    pCurrPos = NextSlash;
                    if (pCurrPos[+1] == '/')
                    {
                        pCurrPos = static_cast<const Char*>(memchr(pCurrPos, '\n', pBufferEnd - pCurrPos));
                    }
                    else if (pCurrPos[+1] == '*')
                    {
                        do
                        {
                            const char* EndSlash = static_cast<const Char*>(memchr(pCurrPos + 1, '/', pBufferEnd - pCurrPos - 1));
                            if (!EndSlash)
                            {
                                ErrorHandler(ParserErrorMessage(MissingEndComment, pBuffer, pCurrPos));
                                return false;
                            }

                            pCurrPos = EndSlash;
                        } while (pCurrPos[-1] != '*');
                    }
                }
                else
                {
                    // Move the current position to the position after the hash
                    pCurrPos          = NextHash;
                    PreprocessorState = AfterHash;
                }
            }
            break;
            case AfterHash:
                // Try to find the 'include' substring in the buffer if the current position is after the hash
                if (!isspace(*pCurrPos))
                {
                    static constexpr auto Len = 7;
                    if (strncmp(pCurrPos, "include", Len) == 0)
                    {
                        PreprocessorState = AfterInclude;
                        pCurrPos += Len;
                    }
                    else
                    {
                        PreprocessorState = None;
                    }
                }
                break;
            case AfterInclude:
                // Try to find the opening quotes character after the 'include' substring
                if (!isspace(*pCurrPos))
                {
                    if (*pCurrPos == '"')
                    {
                        Start             = pCurrPos - pBuffer + 1;
                        PreprocessorState = InsideIncludeQuotes;
                    }
                    else if (*pCurrPos == '<')
                    {
                        Start             = pCurrPos - pBuffer + 1;
                        PreprocessorState = InsideIncludeAngleBrackets;
                    }
                    else
                    {
                        ErrorHandler(ParserErrorMessage(MissingOpeningSymbol, pBuffer, pCurrPos));
                        return false;
                    }
                }
                break;
            case InsideIncludeQuotes:
                // Try to find the closing quotes after the opening quotes and extract the substring for IncludeHandler(...)
                switch (*pCurrPos)
                {
                    case '\n':
                        ErrorHandler(ParserErrorMessage(MissingClosingQuote, pBuffer, pCurrPos));
                        return false;
                    case '"':
                        IncludeHandler(std::string{pBuffer + Start, pCurrPos}, NextHash - pBuffer, pCurrPos - pBuffer + 1);
                        PreprocessorState = None;
                        break;
                }
                break;
            case InsideIncludeAngleBrackets:
                // Try to find the closing quotes after the opening quotes and extract the substring for IncludeHandler(...)
                switch (*pCurrPos)
                {
                    case '\n':
                        ErrorHandler(ParserErrorMessage(MissingClosingBracket, pBuffer, pCurrPos));
                        return false;
                    case '>':
                        IncludeHandler(std::string{pBuffer + Start, pCurrPos}, NextHash - pBuffer, pCurrPos - pBuffer + 1);
                        PreprocessorState = None;
                        break;
                }
                break;
        }
        ++pCurrPos;
    }

    switch (PreprocessorState)
    {
        case None:
        case AfterHash:
            return true;
        case AfterInclude:
            ErrorHandler(ParserErrorMessage(MissingOpeningSymbol, pBuffer, pCurrPos));
            return false;
        case InsideIncludeQuotes:
            ErrorHandler(ParserErrorMessage(MissingClosingQuote, pBuffer, pCurrPos));
            return false;
        case InsideIncludeAngleBrackets:
            ErrorHandler(ParserErrorMessage(MissingClosingBracket, pBuffer, pCurrPos));
            return false;
        default:
            UNEXPECTED("Unknown preprocessor state");
            return false;
    }
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
void ProcessShaderIncludesImpl(const ShaderCreateInfo& ShaderCI, std::unordered_set<std::string>& Includes, IncludeHandlerType IncludeHandler) noexcept(false)
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

bool ProcessShaderIncludes(const ShaderCreateInfo& ShaderCI, std::function<void(const ShaderIncludePreprocessInfo&)> IncludeHandler)
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
                // Process the #inlcude directive
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
        LOG_ERROR_MESSAGE("Failed to unroll includes in ", ErrInfo.first, ": ", ErrInfo.second);
        return "";
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to unroll includes in shader '", (ShaderCI.Desc.Name != nullptr ? ShaderCI.Desc.Name : ""), "'.");
        return "";
    }
}

} // namespace Diligent
