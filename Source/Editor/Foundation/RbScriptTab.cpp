// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "RbScriptTab.h"

#include "../Core/IniHelpers.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RbScript/RbScriptLexer.h>
#include <Urho3D/SystemUI/Widgets.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_Compile = EditorHotkey{"RbScript.Compile"}.Ctrl().Press(KEY_B);
const auto Hotkey_Save = EditorHotkey{"RbScript.Save"}.Ctrl().Press(KEY_S);

ImVec4 TokenColor(RbScriptTokenKind kind)
{
    switch (kind)
    {
    case RbScriptTokenKind::Module:
    case RbScriptTokenKind::Use:
    case RbScriptTokenKind::Script:
    case RbScriptTokenKind::Fn:
    case RbScriptTokenKind::On:
    case RbScriptTokenKind::Async:
    case RbScriptTokenKind::Await:
    case RbScriptTokenKind::Return:
    case RbScriptTokenKind::If:
    case RbScriptTokenKind::Else:
    case RbScriptTokenKind::While:
    case RbScriptTokenKind::For:
    case RbScriptTokenKind::In:
    case RbScriptTokenKind::Let:
    case RbScriptTokenKind::Var:
    case RbScriptTokenKind::Const:
    case RbScriptTokenKind::Struct:
    case RbScriptTokenKind::Enum:
    case RbScriptTokenKind::Class:
    case RbScriptTokenKind::Signal:
    case RbScriptTokenKind::Emit:
    case RbScriptTokenKind::Match:
    case RbScriptTokenKind::Break:
    case RbScriptTokenKind::Continue:
    case RbScriptTokenKind::Public:
    case RbScriptTokenKind::Private:
    case RbScriptTokenKind::Static:
        return ImVec4{0.95f, 0.55f, 0.25f, 1.0f};
    case RbScriptTokenKind::True:
    case RbScriptTokenKind::False:
    case RbScriptTokenKind::Null:
        return ImVec4{0.70f, 0.75f, 1.0f, 1.0f};
    case RbScriptTokenKind::IntegerLiteral:
    case RbScriptTokenKind::FloatLiteral:
        return ImVec4{0.35f, 0.85f, 0.75f, 1.0f};
    case RbScriptTokenKind::StringLiteral:
        return ImVec4{0.45f, 0.90f, 0.45f, 1.0f};
    case RbScriptTokenKind::Identifier:
        return ImVec4{0.90f, 0.90f, 0.92f, 1.0f};
    default:
        return ImVec4{0.72f, 0.75f, 0.80f, 1.0f};
    }
}

bool IsAbsoluteFileName(const ea::string& path)
{
    return path.starts_with("/") || (path.size() > 1 && path[1] == ':');
}

ea::string ProjectFileName(Project* project, const ea::string& resourceName)
{
    if (!project || IsAbsoluteFileName(resourceName))
        return resourceName;
    return AddTrailingSlash(project->GetProjectPath()) + resourceName;
}

class RbScriptSourceSnapshotAction final : public EditorAction
{
public:
    RbScriptSourceSnapshotAction(RbScriptTab* tab, const ea::string& before, const ea::string& after)
        : tab_(tab)
        , before_(before)
        , after_(after)
    {
    }

    void Redo() const override
    {
        if (tab_)
            tab_->ApplySourceSnapshot(after_);
    }

    void Undo() const override
    {
        if (tab_)
            tab_->ApplySourceSnapshot(before_);
    }

private:
    WeakPtr<RbScriptTab> tab_;
    ea::string before_;
    ea::string after_;
};

}

void Foundation_RbScriptTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<RbScriptTab>(context));
}

RbScriptTab::RbScriptTab(Context* context)
    : ResourceEditorTab(context, "rbscript", "4e4bd5ec-5085-4f88-89f8-2fbead3c7a9f",
          EditorTabFlag::None, EditorTabPlacement::DockCenter)
{
    BindHotkey(Hotkey_Compile, &RbScriptTab::CompileActiveDocument);
    BindHotkey(Hotkey_Save, &RbScriptTab::SaveCurrentResource);
}

bool RbScriptTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<RbScriptResource>() || desc.HasExtension(".rbscript");
}

RbScriptTab::Document* RbScriptTab::GetActiveDocument()
{
    auto iter = documents_.find(GetActiveResourceName());
    return iter != documents_.end() ? &iter->second : nullptr;
}

const RbScriptTab::Document* RbScriptTab::GetActiveDocument() const
{
    auto iter = documents_.find(GetActiveResourceName());
    return iter != documents_.end() ? &iter->second : nullptr;
}

ea::string RbScriptTab::GetActiveSource() const
{
    return activeSource_;
}

void RbScriptTab::SetActiveSource(const ea::string& source)
{
    activeSource_ = source;
    if (Document* document = GetActiveDocument())
        document->source = source;
}

void RbScriptTab::TokenizeDocument(Document& document)
{
    RbScriptLexer lexer(document.source, GetActiveResourceName());
    document.tokens = lexer.Tokenize();
    document.diagnostics = lexer.GetDiagnostics();
}

void RbScriptTab::RefreshDocument(const ea::string& resourceName, bool compile)
{
    auto iter = documents_.find(resourceName);
    if (iter == documents_.end())
        return;

    Document& document = iter->second;
    RbScriptLexer lexer(document.source, resourceName);
    document.tokens = lexer.Tokenize();
    document.diagnostics = lexer.GetDiagnostics();
    document.compiled = false;

    if (compile && document.diagnostics.empty())
    {
        RbScriptResource resource(context_);
        document.compiled = resource.CompileSource(document.source, resourceName);
        document.diagnostics = resource.GetDiagnostics();
    }

    if (resourceName == GetActiveResourceName())
        activeSource_ = document.source;
}

void RbScriptTab::ApplySource(const ea::string& resourceName, const ea::string& source, bool compile)
{
    auto iter = documents_.find(resourceName);
    if (iter == documents_.end())
        return;
    iter->second.source = source;
    RefreshDocument(resourceName, compile);
}

void RbScriptTab::LoadDocument(const ea::string& resourceName)
{
    Document& document = documents_[resourceName];
    document = Document{};

    auto cache = GetSubsystem<ResourceCache>();
    AbstractFilePtr file = cache ? cache->GetFile(resourceName, false) : nullptr;
    if (file && file->IsOpen())
    {
        const unsigned size = file->GetSize();
        document.source.resize(size);
        if (size && file->Read(&document.source[0], size) != size)
            document.source.clear();
    }

    RefreshDocument(resourceName, true);
    activeSource_ = document.source;
    status_ = Format("Loaded {}", resourceName);
}

void RbScriptTab::SaveDocument(const ea::string& resourceName)
{
    auto iter = documents_.find(resourceName);
    if (iter == documents_.end())
        return;

    const ea::string fileName = ProjectFileName(GetProject(), resourceName);
    File file(context_, fileName, FILE_WRITE);
    if (!file.IsOpen())
    {
        status_ = Format("Unable to save {}", resourceName);
        return;
    }

    const ea::string& source = iter->second.source;
    if (!source.empty() && file.Write(source.data(), source.size()) != source.size())
    {
        status_ = Format("Unable to write {}", resourceName);
        return;
    }

    RefreshDocument(resourceName, true);
    status_ = Format("Saved {}", resourceName);
}

void RbScriptTab::PushSourceEdit(const ea::string& before, const ea::string& after)
{
    if (before == after || GetActiveResourceName().empty())
        return;
    PushAction<RbScriptSourceSnapshotAction>(this, before, after);
}

void RbScriptTab::ApplySourceSnapshot(const ea::string& source)
{
    if (GetActiveResourceName().empty())
        return;
    ApplySource(GetActiveResourceName(), source, autoCompile_);
    status_ = "rbscript edit restored";
}

void RbScriptTab::CompileActiveDocument()
{
    if (GetActiveResourceName().empty())
        return;
    RefreshDocument(GetActiveResourceName(), true);
    const Document* document = GetActiveDocument();
    status_ = document && document->compiled ? "rbscript compiled successfully" : "rbscript compilation failed";
}

void RbScriptTab::RenderDiagnostics(const Document& document)
{
    if (!showDiagnostics_)
        return;

    ui::Separator();
    ui::Text("Diagnostics (%u)", static_cast<unsigned>(document.diagnostics.size()));
    for (unsigned i = 0; i < document.diagnostics.size(); ++i)
    {
        const RbScriptDiagnostic& diagnostic = document.diagnostics[i];
        const bool error = diagnostic.severity == RbScriptDiagnosticSeverity::Error;
        const ImVec4 color = error ? ImVec4{1.0f, 0.35f, 0.35f, 1.0f} : ImVec4{1.0f, 0.80f, 0.30f, 1.0f};
        ui::PushStyleColor(ImGuiCol_Text, color);
        ui::Text("%s:%u:%u [%s] %s", diagnostic.file.c_str(), diagnostic.span.begin.line,
            diagnostic.span.begin.column, diagnostic.code.c_str(), diagnostic.message.c_str());
        ui::PopStyleColor();
    }
}

void RbScriptTab::RenderTokenPreview(const Document& document)
{
    if (!showPreview_)
        return;

    ui::BeginChild("##RbScriptPreview", ImVec2{0.0f, 190.0f}, true);
    ui::Text("Lexical preview");
    ui::Separator();

    unsigned offset = 0;
    for (const RbScriptToken& token : document.tokens)
    {
        if (token.kind == RbScriptTokenKind::EndOfFile)
            break;

        if (token.span.begin.offset > offset)
        {
            const ea::string whitespace = document.source.substr(offset, token.span.begin.offset - offset);
            ui::TextUnformatted(whitespace.c_str());
            if (!whitespace.empty() && whitespace.back() != '\n')
                ui::SameLine(0.0f, 0.0f);
        }

        ui::TextColored(TokenColor(token.kind), "%s", token.lexeme.c_str());
        if (token.lexeme.find('\n') == ea::string::npos)
            ui::SameLine(0.0f, 0.0f);
        offset = token.span.end.offset;
    }
    ui::EndChild();
}

void RbScriptTab::RenderAutocomplete(const Document& document)
{
    (void)document;
    if (searchText_.empty())
        return;

    static const char* suggestions[] = {
        "module", "use", "script", "fn", "on", "async", "let", "var", "const",
        "if", "else", "while", "return", "emit", "Vector2", "Vector3", "Quaternion",
        "Color", "Node", "Component", "Resource", "Variant", "Array", "Map", "Optional",
    };

    ui::Text("Suggestions");
    for (const char* suggestion : suggestions)
    {
        if (ea::string(suggestion).find(searchText_) != ea::string::npos)
        {
            ui::SameLine();
            if (ui::SmallButton(suggestion))
                status_ = Format("Suggestion: {}", suggestion);
        }
    }
}

void RbScriptTab::RenderContent()
{
    Document* document = GetActiveDocument();
    if (!document)
    {
        ui::TextWrapped("Open a .rbscript resource from the Resource Browser to start editing.");
        return;
    }

    const ea::string before = activeSource_;
    const ImVec2 editorSize = ImVec2{0.0f, showPreview_ ? -210.0f : -150.0f};
    ImFont* monoFont = Project::GetMonoFont();
    if (monoFont)
        ui::PushFont(monoFont);

    const bool changed = ui::InputTextMultiline("##RbScriptSource", &activeSource_, editorSize,
        ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackAlways);
    sourceFocused_ = ui::IsItemActive();
    if (monoFont)
        ui::PopFont();

    if (changed)
    {
        document->source = activeSource_;
        RefreshDocument(GetActiveResourceName(), autoCompile_);
        PushSourceEdit(before, activeSource_);
    }

    RenderAutocomplete(*document);
    RenderTokenPreview(*document);
    RenderDiagnostics(*document);
}

void RbScriptTab::RenderToolbar()
{
    if (ui::Button("Compile"))
        CompileActiveDocument();
    ui::SameLine();
    if (ui::Button("Save"))
        SaveCurrentResource();
    ui::SameLine();
    ui::Checkbox("Auto compile", &autoCompile_);
    ui::SameLine();
    ui::Checkbox("Preview", &showPreview_);
    ui::SameLine();
    ui::Checkbox("Diagnostics", &showDiagnostics_);
    ui::SameLine();
    ui::Text("%s", status_.c_str());
}

void RbScriptTab::RenderContextMenuItems()
{
    ResourceEditorTab::RenderContextMenuItems();
    contextMenuSeparator_.Reset();
    if (ui::MenuItem("Compile rbscript", GetHotkeyLabel(Hotkey_Compile).c_str()))
        CompileActiveDocument();
    ui::MenuItem("Lexical preview", nullptr, &showPreview_);
    ui::MenuItem("Diagnostics", nullptr, &showDiagnostics_);
}

void RbScriptTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    ResourceEditorTab::WriteIniSettings(output);
    WriteStringToIni(output, "AutoCompile", Format("{}", autoCompile_ ? 1 : 0));
    WriteStringToIni(output, "Preview", Format("{}", showPreview_ ? 1 : 0));
    WriteStringToIni(output, "Diagnostics", Format("{}", showDiagnostics_ ? 1 : 0));
}

void RbScriptTab::ReadIniSettings(const char* line)
{
    ResourceEditorTab::ReadIniSettings(line);
    if (const auto value = ReadStringFromIni(line, "AutoCompile"))
        autoCompile_ = ToInt(*value) != 0;
    if (const auto value = ReadStringFromIni(line, "Preview"))
        showPreview_ = ToInt(*value) != 0;
    if (const auto value = ReadStringFromIni(line, "Diagnostics"))
        showDiagnostics_ = ToInt(*value) != 0;
}

void RbScriptTab::OnResourceLoaded(const ea::string& resourceName)
{
    LoadDocument(resourceName);
}

void RbScriptTab::OnResourceUnloaded(const ea::string& resourceName)
{
    documents_.erase(resourceName);
    if (resourceName == GetActiveResourceName())
        activeSource_.clear();
}

void RbScriptTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
    (void)oldResourceName;
    auto iter = documents_.find(newResourceName);
    activeSource_ = iter != documents_.end() ? iter->second.source : ea::string{};
    status_ = newResourceName.empty() ? ea::string{} : Format("Active: {}", newResourceName);
}

void RbScriptTab::OnResourceSaved(const ea::string& resourceName)
{
    SaveDocument(resourceName);
}

void RbScriptTab::OnResourceShallowSaved(const ea::string& resourceName)
{
    (void)resourceName;
}

} // namespace Urho3D
