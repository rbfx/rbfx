// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptResource.h"

#include "RbScriptLexer.h"
#include "RbScriptParser.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/Serializer.h>

namespace Urho3D
{

namespace
{

void AppendDiagnostics(ea::vector<RbScriptDiagnostic>& destination,
    const ea::vector<RbScriptDiagnostic>& source)
{
    destination.insert(destination.end(), source.begin(), source.end());
}

bool HasErrors(const ea::vector<RbScriptDiagnostic>& diagnostics)
{
    for (const RbScriptDiagnostic& diagnostic : diagnostics)
    {
        if (diagnostic.severity == RbScriptDiagnosticSeverity::Error)
            return true;
    }
    return false;
}

} // namespace

RbScriptResource::RbScriptResource(Context* context)
    : Resource(context)
{
}

void RbScriptResource::RegisterObject(Context* context)
{
    context->RegisterFactory<RbScriptResource>();
}

bool RbScriptResource::BeginLoad(Deserializer& source)
{
    const unsigned size = source.GetSize();
    ea::string sourceText;
    sourceText.resize(size);
    if (size && source.Read(sourceText.data(), size) != size)
    {
        URHO3D_LOGERROR("Could not read rbscript resource '{}'.", source.GetName());
        return false;
    }

    const bool success = CompileBuffer(sourceText, source.GetName());
    if (!success)
    {
        for (const RbScriptDiagnostic& diagnostic : diagnostics_)
        {
            URHO3D_LOGERROR("rbscript {} [{}:{}:{}]: {}", ToString(diagnostic.severity), diagnostic.code,
                diagnostic.span.begin.line, diagnostic.span.begin.column, diagnostic.message);
        }
        return false;
    }

    if (FileSystem* fileSystem = GetSubsystem<FileSystem>())
    {
        const ea::string& absoluteName = GetAbsoluteFileName().empty() ? source.GetName() : GetAbsoluteFileName();
        sourceTimeStamp_ = fileSystem->GetLastModifiedTime(absoluteName);
    }
    return true;
}

bool RbScriptResource::Save(Serializer& dest) const
{
    return dest.Write(source_.data(), source_.size()) == source_.size();
}

bool RbScriptResource::CompileSource(const ea::string& source, const ea::string& fileName)
{
    const bool success = CompileBuffer(source, fileName);
    if (success && !fileName.empty())
    {
        if (FileSystem* fileSystem = GetSubsystem<FileSystem>())
            sourceTimeStamp_ = fileSystem->GetLastModifiedTime(fileName);
    }
    return success;
}

bool RbScriptResource::ReloadIfChanged()
{
    const ea::string& absoluteName = GetAbsoluteFileName();
    const ea::string& resourceName = GetName();
    const ea::string fileName = absoluteName.empty() ? resourceName : absoluteName;
    if (fileName.empty())
        return false;

    FileSystem* fileSystem = GetSubsystem<FileSystem>();
    if (!fileSystem || !fileSystem->FileExists(fileName))
        return false;

    const FileTime currentTimeStamp = fileSystem->GetLastModifiedTime(fileName);
    if (currentTimeStamp == 0 || currentTimeStamp == sourceTimeStamp_)
        return compiled_;

    ea::string source;
    if (!ReadSourceFile(source))
        return false;

    return CompileBuffer(source, fileName);
}

bool RbScriptResource::CompileBuffer(const ea::string& source, const ea::string& fileName)
{
    ea::vector<RbScriptDiagnostic> diagnostics;

    RbScriptLexer lexer(source, fileName);
    const ea::vector<RbScriptToken> tokens = lexer.Tokenize();
    AppendDiagnostics(diagnostics, lexer.GetDiagnostics());
    if (HasErrors(diagnostics))
    {
        diagnostics_ = ea::move(diagnostics);
        return false;
    }

    RbScriptParser parser(tokens, fileName);
    RbScriptModule module = parser.ParseModule();
    AppendDiagnostics(diagnostics, parser.GetDiagnostics());
    if (HasErrors(diagnostics) || !module.IsValid())
    {
        diagnostics_ = ea::move(diagnostics);
        if (diagnostics_.empty())
        {
            RbScriptDiagnostic diagnostic;
            diagnostic.severity = RbScriptDiagnosticSeverity::Error;
            diagnostic.file = fileName;
            diagnostic.code = "RS100";
            diagnostic.message = "The rbscript module is empty or invalid.";
            diagnostics_.push_back(ea::move(diagnostic));
        }
        return false;
    }

    RbScriptTypeRegistry registry;
    RbScriptTypeChecker typeChecker(registry);
    if (!typeChecker.Check(module))
    {
        AppendDiagnostics(diagnostics, typeChecker.GetDiagnostics());
        diagnostics_ = ea::move(diagnostics);
        return false;
    }
    AppendDiagnostics(diagnostics, typeChecker.GetDiagnostics());

    RbScriptCompiler compiler(&registry);
    RbScriptChunk compiledChunk = compiler.Compile(module);
    AppendDiagnostics(diagnostics, compiler.GetDiagnostics());
    if (compiler.HadError() || HasErrors(diagnostics))
    {
        diagnostics_ = ea::move(diagnostics);
        return false;
    }

    source_ = source;
    chunk_ = ea::move(compiledChunk);
    diagnostics_ = ea::move(diagnostics);
    compiled_ = true;
    SetMemoryUse(static_cast<unsigned>(source_.size()
        + chunk_.instructions.size() * sizeof(RbScriptInstruction)
        + chunk_.constants.size() * sizeof(RbScriptConstant)));

    if (FileSystem* fileSystem = GetSubsystem<FileSystem>(); !fileName.empty())
        sourceTimeStamp_ = fileSystem ? fileSystem->GetLastModifiedTime(fileName) : 0;
    return true;
}

bool RbScriptResource::ReadSourceFile(ea::string& source) const
{
    const ea::string& absoluteName = GetAbsoluteFileName();
    const ea::string& resourceName = GetName();
    const ea::string fileName = absoluteName.empty() ? resourceName : absoluteName;
    if (fileName.empty())
        return false;

    File file(GetContext(), fileName, FILE_READ);
    if (!file.IsOpen())
        return false;
    source = file.ReadText();
    return true;
}

} // namespace Urho3D
