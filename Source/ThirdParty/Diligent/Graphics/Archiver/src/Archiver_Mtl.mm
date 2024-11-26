/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "ArchiverImpl.hpp"
#include "Archiver_Inc.hpp"

#include <thread>
#include <sstream>
#include <filesystem>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "RenderDeviceMtlImpl.hpp"
#include "PipelineResourceSignatureMtlImpl.hpp"
#include "PipelineStateMtlImpl.hpp"
#include "ShaderMtlImpl.hpp"
#include "RenderPassMtlImpl.hpp"
#include "DeviceObjectArchiveMtlImpl.hpp"
#include "SerializedPipelineStateImpl.hpp"
#include "DeviceObjectArchiveMtlImpl.hpp"

#include "FileSystem.hpp"
#include "FileWrapper.hpp"
#include "DataBlobImpl.hpp"

namespace filesystem = std::__fs::filesystem;

namespace Diligent
{

template <>
struct SerializedResourceSignatureImpl::SignatureTraits<PipelineResourceSignatureMtlImpl>
{
    static constexpr DeviceType Type = DeviceType::Metal_MacOS;

    template <SerializerMode Mode>
    using PRSSerializerType = PRSSerializerMtl<Mode>;
};

namespace
{

std::string GetTmpFolder()
{
    const auto ProcId   = getpid();
    const auto ThreadId = std::this_thread::get_id();
    const auto Slash    = FileSystem::SlashSymbol;

    std::string TmpDir{filesystem::temp_directory_path().c_str()};
    if (TmpDir.back() != Slash)
        TmpDir.push_back(Slash);

    std::stringstream FolderPath;
    FolderPath << TmpDir
               << "DiligentArchiver-"
               << ProcId << '-'
               << ThreadId << Slash;
    return FolderPath.str();
}

struct ShaderStageInfoMtl
{
    ShaderStageInfoMtl() {}

    ShaderStageInfoMtl(const SerializedShaderImpl* _pShader) :
        Type{_pShader->GetDesc().ShaderType},
        pShader{_pShader}
    {}

    // Needed only for ray tracing
    void Append(const SerializedShaderImpl*) {}

    constexpr Uint32 Count() const { return 1; }

    SHADER_TYPE                 Type    = SHADER_TYPE_UNKNOWN;
    const SerializedShaderImpl* pShader = nullptr;
};

#ifdef DILIGENT_DEBUG
inline SHADER_TYPE GetShaderStageType(const ShaderStageInfoMtl& Stage)
{
    return Stage.Type;
}
#endif


struct TmpDirRemover
{
    explicit TmpDirRemover(const std::string& _Path) noexcept :
        Path{_Path}
    {}

    ~TmpDirRemover()
    {
        if (!Path.empty())
        {
            filesystem::remove_all(Path.c_str());
        }
    }
private:
    const std::string Path;
};

#define LOG_ERRNO_MESSAGE(...)                                           \
    do                                                                   \
    {                                                                    \
        char ErrorStr[512];                                              \
        strerror_r(errno, ErrorStr, sizeof(ErrorStr));                   \
        LOG_ERROR_MESSAGE(__VA_ARGS__, " Error description: ", ErrorStr);\
    } while (false)

bool SaveMslToFile(const std::string& MslSource, const std::string& MetalFile)
{
    auto* File = fopen(MetalFile.c_str(), "wb");
    if (File == nullptr)
    {
        LOG_ERRNO_MESSAGE("failed to open file '", MetalFile,"' to save Metal shader source.");
        return false;
    }

    if (fwrite(MslSource.c_str(), sizeof(MslSource[0]) * MslSource.size(), 1, File) != 1)
    {
        LOG_ERRNO_MESSAGE("failed to save Metal shader source to file '", MetalFile, "'.");
        return false;
    }

    fclose(File);

    return true;
}

// Runs a custom MSL processing command
bool RunPreprocessMslCmd(const std::string& MslPreprocessorCmd, const std::string& MetalFile)
{
    auto cmd{MslPreprocessorCmd};
    cmd += " \"";
    cmd += MetalFile;
    cmd += '\"';
    FILE* Pipe = FileSystem::popen(cmd.c_str(), "r");
    if (Pipe == nullptr)
    {
        LOG_ERRNO_MESSAGE("failed to run command-line Metal shader compiler with command line \"", cmd, "'.");
        return false;
    }

    char Output[512];
    while (fgets(Output, _countof(Output), Pipe) != nullptr)
        printf("%s", Output);

    auto status = FileSystem::pclose(Pipe);
    if (status != 0)
    {
        // errno is not useful
        LOG_ERROR_MESSAGE("failed to close msl preprocessor process (error code: ", status, ").");
        return false;
    }

    return true;
}

// Compiles MSL into a metal library using xcrun
// https://developer.apple.com/documentation/metal/libraries/generating_and_loading_a_metal_library_symbol_file
bool CompileMsl(const std::string& CompileOptions,
                const std::string& MetalFile,
                const std::string& MetalLibFile) noexcept(false)
{
    String cmd{"xcrun "};
    cmd += CompileOptions;
    cmd += " \"" + MetalFile + "\" -o \"" + MetalLibFile + '\"';

    FILE* Pipe = FileSystem::popen(cmd.c_str(), "r");
    if (Pipe == nullptr)
    {
        LOG_ERRNO_MESSAGE("failed to compile MSL source with command line \"", cmd, '\"');
        return false;
    }

    char Output[512];
    while (fgets(Output, _countof(Output), Pipe) != nullptr)
        printf("%s", Output);

    auto status = FileSystem::pclose(Pipe);
    if (status != 0)
    {
        // errno is not useful
        LOG_ERROR_MESSAGE("failed to close xcrun process (error code: ", status, ").");
        return false;
    }

    return true;
}

RefCntAutoPtr<DataBlobImpl> ReadFile(const char* FilePath)
{
    FileWrapper File{FilePath, EFileAccessMode::Read};
    if (!File)
    {
        LOG_ERRNO_MESSAGE("Failed to open file '", FilePath, "'.");
        return {};
    }

    auto pFileData = DataBlobImpl::Create();
    if (!File->Read(pFileData))
    {
        LOG_ERRNO_MESSAGE("Failed to read '", FilePath, "'.");
        return {};
    }

    return pFileData;
}

#undef LOG_ERRNO_MESSAGE

void PreprocessMslSource(const std::string& MslPreprocessorCmd, const char* ShaderName, std::string& MslSource) noexcept(false)
{
    if (MslPreprocessorCmd.empty())
        return;
    
    VERIFY_EXPR(ShaderName != nullptr);
    
    const auto TmpFolder = GetTmpFolder();
    filesystem::create_directories(TmpFolder);
    TmpDirRemover DirRemover{TmpFolder};

    const auto MetalFile = TmpFolder + ShaderName + ".metal";

    // Save MSL source to a file
    if (!SaveMslToFile(MslSource, MetalFile))
        LOG_ERROR_AND_THROW("Failed to save MSL source to a temp file for shader '", ShaderName,"'.");

    // Run the preprocessor
    if (!RunPreprocessMslCmd(MslPreprocessorCmd, MetalFile))
        LOG_ERROR_AND_THROW("Failed to preprocess MSL source for shader '", ShaderName,"'.");

    // Read processed MSL source back
    auto pProcessedMsl = ReadFile(MetalFile.c_str());
    if (!pProcessedMsl)
        LOG_ERROR_AND_THROW("Failed to read preprocessed MSL source for shader '", ShaderName,"'.");

    const auto* pMslStr = pProcessedMsl->GetConstDataPtr<char>();
    MslSource = {pMslStr, pMslStr + pProcessedMsl->GetSize()};
}

struct CompiledShaderMtl final : SerializedShaderImpl::CompiledShader
{
    ShaderMtlImpl ShaderMtl;

    CompiledShaderMtl(
        IReferenceCounters*              pRefCounters,
        const ShaderCreateInfo&          ShaderCI,
        const ShaderMtlImpl::CreateInfo& MtlShaderCI,
        IRenderDevice*                   pRenderDeviceMtl):
        ShaderMtl
        {
            pRefCounters,
            static_cast<RenderDeviceMtlImpl*>(pRenderDeviceMtl),
            ShaderCI,
            MtlShaderCI,
            true
        }
    {
    }

    virtual SerializedData Serialize(ShaderCreateInfo ShaderCI) const override final
    {
        SerializedData ShaderData = SerializeMslSourceAndMtlArchiveData(ShaderMtl);

        ShaderCI.FilePath       = nullptr;
        ShaderCI.Source         = nullptr;
        ShaderCI.Macros         = {};
        ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MSL_VERBATIM;
        ShaderCI.ByteCode       = ShaderData.Ptr();
        ShaderCI.ByteCodeSize   = ShaderData.Size();

        return SerializedShaderImpl::SerializeCreateInfo(ShaderCI);
    }

    virtual IShader* GetDeviceShader() override final
    {
        return &ShaderMtl;
    }

    virtual bool IsCompiling() const override final
    {
        return ShaderMtl.IsCompiling();
    }

    virtual RefCntAutoPtr<IAsyncTask> GetCompileTask() const override final
    {
        return ShaderMtl.GetCompileTask();
    }
};

inline const ParsedMSLInfo* GetParsedMsl(const SerializedShaderImpl* pShader, SerializedShaderImpl::DeviceType Type)
{
    const auto* pCompiledShaderMtl = pShader->GetShader<const CompiledShaderMtl>(Type);
    return pCompiledShaderMtl != nullptr ? &pCompiledShaderMtl->ShaderMtl.GetParsedMsl() : nullptr;
}


struct CompileMtlShaderAttribs
{
    const SerializedShaderImpl::DeviceType DevType;
    const SerializationDeviceImpl*         pSerializationDevice;

    const SerializedShaderImpl* pSerializedShader;
    const IRenderPass*          pRenderPass;
    const Uint32                SubpassIndex;

    const char*         PSOName;
    const std::string&  DumpFolder;

    const SignatureArray<PipelineResourceSignatureMtlImpl>& Signatures;
    const Uint32                                            SignatureCount;

    const std::array<MtlResourceCounters, MAX_RESOURCE_SIGNATURES>& BaseBindings;
};

SerializedData CompileMtlShader(const CompileMtlShaderAttribs& Attribs) noexcept(false)
{
    using DeviceType = SerializedShaderImpl::DeviceType;

    VERIFY_EXPR(Attribs.SignatureCount > 0);
    const auto* PSOName = Attribs.PSOName != nullptr ? Attribs.PSOName : "<unknown>";

    const auto& ShDesc     = Attribs.pSerializedShader->GetDesc();
    const auto* ShaderName = ShDesc.Name;
    VERIFY_EXPR(ShaderName != nullptr);

    const std::string WorkingFolder =
        [&](){
            if (Attribs.DumpFolder.empty())
                return GetTmpFolder();

            auto Folder = Attribs.DumpFolder;
            if (Folder.back() != FileSystem::SlashSymbol)
                Folder += FileSystem::SlashSymbol;

            return Folder;
        }();
    filesystem::create_directories(WorkingFolder);

    TmpDirRemover DirRemover{Attribs.DumpFolder.empty() ? WorkingFolder : ""};

    const auto MetalFile    = WorkingFolder + ShaderName + ".metal";
    const auto MetalLibFile = WorkingFolder + ShaderName + ".metallib";

    const auto* pCompiledShader = Attribs.pSerializedShader->GetShader<const CompiledShaderMtl>(Attribs.DevType);

    const auto& MslData   = pCompiledShader->ShaderMtl.GetMslData();
    const auto& ParsedMsl = pCompiledShader->ShaderMtl.GetParsedMsl();

#define LOG_PATCH_SHADER_ERROR_AND_THROW(...)\
    LOG_ERROR_AND_THROW("Failed to patch shader '", ShaderName, "' for PSO '", PSOName, "': ", ##__VA_ARGS__)

    using IORemappingMode = ShaderMtlImpl::ArchiveData::IORemappingMode;
    auto  IORemapping     = IORemappingMode::Undefined;

    std::string MslSource;
    if (ParsedMsl.pParser != nullptr)
    {
        // Same logic as in PipelineStateMtlImpl::GetPatchedMtlFunction
        
        MSLParser::ResourceMapType ResRemapping;
        PipelineStateMtlImpl::GetResourceMap({
            ParsedMsl,
            Attribs.Signatures.data(),
            Attribs.SignatureCount,
            Attribs.BaseBindings.data(),
            ShDesc,
            PSOName},
            ResRemapping); // may throw exception

        if (ShDesc.ShaderType == SHADER_TYPE_VERTEX)
        {
            // Remap vertex input attributes
            PipelineStateMtlImpl::GetVSInputMap({
                ParsedMsl,
                MslData
            },
            ResRemapping);
        }
        else if (ShDesc.ShaderType == SHADER_TYPE_PIXEL && Attribs.pRenderPass != nullptr)
        {
            const auto& RPDesc   = Attribs.pRenderPass->GetDesc();
            const auto& Features = Attribs.pSerializationDevice->GetDeviceInfo().Features;

            RenderPassMtlImpl RenderPassMtl{RPDesc, Features};
            PipelineStateMtlImpl::GetPSInputOutputMap({
                    ParsedMsl,
                    RenderPassMtl,
                    ShDesc,
                    Attribs.SubpassIndex
                },
                ResRemapping);

            if (RPDesc.SubpassCount > 1)
            {
                IORemapping = Features.SubpassFramebufferFetch ?
                    IORemappingMode::RemappedFetch :
                    IORemappingMode::Original;
            }
        }

        MslSource = ParsedMsl.pParser->RemapResources(ResRemapping);
        if (MslSource.empty())
            LOG_PATCH_SHADER_ERROR_AND_THROW("Failed to remap MSL resources");
    }
    else
    {
        MslSource = MslData.Source;
    }

    if (!SaveMslToFile(MslSource, MetalFile))
        LOG_PATCH_SHADER_ERROR_AND_THROW("Failed to save MSL source to a temp file.");

    const auto& MtlProps = Attribs.pSerializationDevice->GetMtlProperties();
    // Compile MSL to Metal library
    const auto& CompileOptions = Attribs.DevType == DeviceType::Metal_MacOS ?
        MtlProps.CompileOptionsMacOS :
        MtlProps.CompileOptionsIOS;
    if (!CompileMsl(CompileOptions, MetalFile, MetalLibFile))
        LOG_PATCH_SHADER_ERROR_AND_THROW("Failed to create metal library.");

    // Read the bytecode from metal library
    auto pByteCode = ReadFile(MetalLibFile.c_str());
    if (!pByteCode)
        LOG_PATCH_SHADER_ERROR_AND_THROW("Failed to read Metal shader library.");

#undef LOG_PATCH_SHADER_ERROR_AND_THROW

    // Pack shader Mtl acrhive data into the byte code.
    // The data is unpacked by DearchiverMtlImpl::UnpackShader().
    const ShaderMtlImpl::ArchiveData ShaderMtlArchiveData{
        std::move(ParsedMsl.BufferInfoMap),
        MslData.ComputeGroupSize,
        IORemapping
    };

    SerializedData ShaderData;
    {
        Serializer<SerializerMode::Measure> Ser;
        ShaderMtlSerializer<SerializerMode::Measure>::SerializeBytecode(Ser, pByteCode->GetConstDataPtr(), pByteCode->GetSize(), ShaderMtlArchiveData);
        ShaderData = Ser.AllocateData(GetRawAllocator());
    }

    {
        Serializer<SerializerMode::Write> Ser{ShaderData};
        ShaderMtlSerializer<SerializerMode::Write>::SerializeBytecode(Ser, pByteCode->GetConstDataPtr(), pByteCode->GetSize(), ShaderMtlArchiveData);
        VERIFY_EXPR(Ser.IsEnded());
    }

    return ShaderData;
}

} // namespace

template <typename CreateInfoType>
Uint32 GetSubpassIndex(const CreateInfoType&)
{
    return ~0u;
}
template <>
Uint32 GetSubpassIndex(const GraphicsPipelineStateCreateInfo& CI)
{
    return CI.GraphicsPipeline.SubpassIndex;
}

template <typename CreateInfoType>
void SerializedPipelineStateImpl::PatchShadersMtl(const CreateInfoType& CreateInfo, DeviceType DevType, const std::string& DumpDir) noexcept(false)
{
    VERIFY_EXPR(DevType == DeviceType::Metal_MacOS || DevType == DeviceType::Metal_iOS);

    std::vector<ShaderStageInfoMtl> ShaderStages;
    SHADER_TYPE                     ActiveShaderStages    = SHADER_TYPE_UNKNOWN;
    constexpr bool                  WaitUntilShadersReady = true;
    PipelineStateUtils::ExtractShaders<SerializedShaderImpl>(CreateInfo, ShaderStages, WaitUntilShadersReady, ActiveShaderStages);

    std::vector<const ParsedMSLInfo*> StageResources{ShaderStages.size()};
    for (size_t i = 0; i < StageResources.size(); ++i)
    {
        StageResources[i] = GetParsedMsl(ShaderStages[i].pShader, DevType);
    }

    auto** ppSignatures    = CreateInfo.ppResourceSignatures;
    auto   SignaturesCount = CreateInfo.ResourceSignaturesCount;

    IPipelineResourceSignature* DefaultSignatures[1] = {};
    if (CreateInfo.ResourceSignaturesCount == 0)
    {
        CreateDefaultResourceSignature<PipelineStateMtlImpl, PipelineResourceSignatureMtlImpl>(DevType, CreateInfo.PSODesc, ActiveShaderStages, StageResources);

        DefaultSignatures[0] = m_pDefaultSignature;
        SignaturesCount      = 1;
        ppSignatures         = DefaultSignatures;
    }

    {
        // Sort signatures by binding index.
        // Note that SignaturesCount will be overwritten with the maximum binding index.
        SignatureArray<PipelineResourceSignatureMtlImpl> Signatures      = {};
        SortResourceSignatures(ppSignatures, SignaturesCount, Signatures, SignaturesCount, DevType);

        std::array<MtlResourceCounters, MAX_RESOURCE_SIGNATURES> BaseBindings{};
        MtlResourceCounters                                      CurrBindings{};
        for (Uint32 s = 0; s < SignaturesCount; ++s)
        {
            BaseBindings[s] = CurrBindings;
            const auto& pSignature = Signatures[s];
            if (pSignature != nullptr)
                pSignature->ShiftBindings(CurrBindings);
        }

        VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DevType)].empty());
        for (size_t j = 0; j < ShaderStages.size(); ++j)
        {
            const auto& Stage = ShaderStages[j];
            // Note that patched shader data contains some extra information
            // besides the byte code itself.
            const auto ShaderData = CompileMtlShader({
                DevType,
                m_pSerializationDevice,
                Stage.pShader,
                m_pRenderPass,
                GetSubpassIndex(CreateInfo),
                CreateInfo.PSODesc.Name,
                DumpDir,
                Signatures,
                SignaturesCount,
                BaseBindings,
            }); // May throw

            auto ShaderCI           = Stage.pShader->GetCreateInfo();
            ShaderCI.Source         = nullptr;
            ShaderCI.FilePath       = nullptr;
            ShaderCI.Macros         = {};
            ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_MTLB;
            ShaderCI.ByteCode       = ShaderData.Ptr();
            ShaderCI.ByteCodeSize   = ShaderData.Size();
            SerializeShaderCreateInfo(DevType, ShaderCI);
        }
        VERIFY_EXPR(m_Data.Shaders[static_cast<size_t>(DevType)].size() == ShaderStages.size());
    }
}

INSTANTIATE_PATCH_SHADER_METHODS(PatchShadersMtl, DeviceType DevType, const std::string& DumpDir)
INSTANTIATE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureMtlImpl)

void SerializedShaderImpl::CreateShaderMtl(IReferenceCounters*     pRefCounters,
                                           const ShaderCreateInfo& ShaderCI,
                                           DeviceType              Type,
                                           IDataBlob**             ppCompilerOutput) noexcept(false)
{
    const auto& DeviceInfo       = m_pDevice->GetDeviceInfo();
    const auto& AdapterInfo      = m_pDevice->GetAdapterInfo();
    const auto& MtlProps         = m_pDevice->GetMtlProperties();
    auto*       pRenderDeviceMtl = m_pDevice->GetRenderDevice(RENDER_DEVICE_TYPE_METAL);

    ShaderMtlImpl::CreateInfo MtlShaderCI
    {
        DeviceInfo,
        AdapterInfo,
        nullptr, // pDearchiveData

        // Do not overwrite compiler output from other APIs.
        // TODO: collect all outputs.
        ppCompilerOutput == nullptr || *ppCompilerOutput == nullptr ? ppCompilerOutput : nullptr,

        m_pDevice->GetShaderCompilationThreadPool(),

        std::function<void(std::string&)>{
            [&](std::string& MslSource)
            {
                PreprocessMslSource(MtlProps.MslPreprocessorCmd, ShaderCI.Desc.Name, MslSource);
            }
        }
    };

    CreateShader<CompiledShaderMtl>(Type, pRefCounters, ShaderCI, MtlShaderCI, pRenderDeviceMtl);
}

void SerializationDeviceImpl::GetPipelineResourceBindingsMtl(const PipelineResourceBindingAttribs& Info,
                                                             std::vector<PipelineResourceBinding>& ResourceBindings,
                                                             const Uint32                          MaxBufferArgs)
{
    ResourceBindings.clear();

    std::array<RefCntAutoPtr<PipelineResourceSignatureMtlImpl>, MAX_RESOURCE_SIGNATURES> Signatures = {};

    Uint32 SignaturesCount = 0;
    for (Uint32 i = 0; i < Info.ResourceSignaturesCount; ++i)
    {
        const auto* pSerPRS = ClassPtrCast<SerializedResourceSignatureImpl>(Info.ppResourceSignatures[i]);
        const auto& Desc    = pSerPRS->GetDesc();

        Signatures[Desc.BindingIndex] = pSerPRS->GetDeviceSignature<PipelineResourceSignatureMtlImpl>(DeviceObjectArchive::DeviceType::Metal_MacOS);
        SignaturesCount               = std::max(SignaturesCount, static_cast<Uint32>(Desc.BindingIndex) + 1);
    }

    const auto          ShaderStages        = (Info.ShaderStages == SHADER_TYPE_UNKNOWN ? static_cast<SHADER_TYPE>(~0u) : Info.ShaderStages);
    constexpr auto      SupportedStagesMask = (SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL | SHADER_TYPE_COMPUTE | SHADER_TYPE_TILE);
    MtlResourceCounters BaseBindings{};

    for (Uint32 sign = 0; sign < SignaturesCount; ++sign)
    {
        const auto& pSignature = Signatures[sign];
        if (pSignature == nullptr)
            continue;

        for (Uint32 r = 0; r < pSignature->GetTotalResourceCount(); ++r)
        {
            const auto& ResDesc = pSignature->GetResourceDesc(r);
            const auto& ResAttr = pSignature->GetResourceAttribs(r);
            const auto  Range   = PipelineResourceDescToMtlResourceRange(ResDesc);

            for (auto Stages = ShaderStages & SupportedStagesMask; Stages != 0;)
            {
                const auto ShaderStage = ExtractLSB(Stages);
                const auto ShaderInd   = MtlResourceBindIndices::ShaderTypeToIndex(ShaderStage);
                DEV_CHECK_ERR(ShaderInd < MtlResourceBindIndices::NumShaderTypes,
                              "Unsupported shader stage (", GetShaderTypeLiteralName(ShaderStage), ") for Metal backend");

                if ((ResDesc.ShaderStages & ShaderStage) == 0)
                    continue;

                ResourceBindings.push_back(ResDescToPipelineResBinding(ResDesc, ShaderStage, BaseBindings[ShaderInd][Range] + ResAttr.BindIndices[ShaderInd], 0 /*space*/));
            }
        }
        pSignature->ShiftBindings(BaseBindings);
    }

    // Add vertex buffer bindings.
    // Same as DeviceContextMtlImpl::CommitVertexBuffers()
    if ((Info.ShaderStages & SHADER_TYPE_VERTEX) != 0 && Info.NumVertexBuffers > 0)
    {
        DEV_CHECK_ERR(Info.VertexBufferNames != nullptr, "VertexBufferNames must not be null");

        const auto BaseSlot = MaxBufferArgs - Info.NumVertexBuffers;
        for (Uint32 i = 0; i < Info.NumVertexBuffers; ++i)
        {
            PipelineResourceBinding Dst{};
            Dst.Name         = Info.VertexBufferNames[i];
            Dst.ResourceType = SHADER_RESOURCE_TYPE_BUFFER_SRV;
            Dst.Register     = BaseSlot + i;
            Dst.Space        = 0;
            Dst.ArraySize    = 1;
            Dst.ShaderStages = SHADER_TYPE_VERTEX;
            ResourceBindings.push_back(Dst);
        }
    }
}

} // namespace Diligent
