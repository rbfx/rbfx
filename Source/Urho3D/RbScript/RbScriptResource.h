// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "RbScriptCompiler.h"
#include <Urho3D/IO/ScanFlags.h>
#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{

/// Native rbfx resource that compiles and caches an rbscript source file.
class URHO3D_API RbScriptResource : public Resource
{
    URHO3D_OBJECT(RbScriptResource, Resource)

public:
    explicit RbScriptResource(Context* context);

    /// Register the rbscript resource type in the rbfx object factory.
    static void RegisterObject(Context* context);

    /// Read, parse, type-check and compile an rbscript source stream.
    bool BeginLoad(Deserializer& source) override;
    /// Save the original rbscript source text.
    bool Save(Serializer& dest) const override;

    /// Compile source text directly, useful for editor previews and tests.
    bool CompileSource(const ea::string& source, const ea::string& fileName = {});
    /// Reload the source file when its modification time has changed.
    bool ReloadIfChanged();

    /// Return the original source text.
    const ea::string& GetSource() const { return source_; }
    /// Return the compiled bytecode chunk.
    const RbScriptChunk& GetChunk() const { return chunk_; }
    /// Return diagnostics from the last compilation attempt.
    const ea::vector<RbScriptDiagnostic>& GetDiagnostics() const { return diagnostics_; }
    /// Return whether the last compilation produced a valid chunk.
    bool IsCompiled() const { return compiled_; }
    /// Return the source modification timestamp captured at load time.
    FileTime GetSourceTimeStamp() const { return sourceTimeStamp_; }

private:
    /// Compile a source buffer and replace the cached chunk only on success.
    bool CompileBuffer(const ea::string& source, const ea::string& fileName);
    /// Read source text from the resource's absolute file name.
    bool ReadSourceFile(ea::string& source) const;

    ea::string source_;
    RbScriptChunk chunk_;
    ea::vector<RbScriptDiagnostic> diagnostics_;
    FileTime sourceTimeStamp_{};
    bool compiled_{false};
};

} // namespace Urho3D
