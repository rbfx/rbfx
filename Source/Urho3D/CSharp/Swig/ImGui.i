%module(naturalvar=1) ImGui

%import "Urho3D.i"
%include "generated/ImGui/_pre_include.i"

// Speed boost
%pragma(csharp) imclassclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\ninternal unsafe class"
%pragma(csharp) moduleclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\npublic unsafe partial class"
%typemap(csclassmodifiers) SWIGTYPE "[global::Urho3DNet.Preserve(AllMembers=true)]\npublic unsafe partial class"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1
%{
#include <Urho3D/CSharp/Native/SWIGHelpers.h>
#include <ImGui/imgui.h>
//#include <ImGui/imgui_internal.h>
%}

// imgui defines this to ints
%ignore ImGuiCol;
%ignore ImGuiCond;
%ignore ImGuiDataType;
%ignore ImGuiDir;
%ignore ImGuiKey;
%ignore ImGuiNavInput;
%ignore ImGuiMouseCursor;
%ignore ImGuiStyleVar;
%ignore ImDrawCornerFlags;
%ignore ImDrawListFlags;
%ignore ImFontAtlasFlags;
%ignore ImGuiBackendFlags;
%ignore ImGuiColorEditFlags;
%ignore ImGuiConfigFlags;
%ignore ImGuiComboFlags;
%ignore ImGuiDockNodeFlags;
%ignore ImGuiDragDropFlags;
%ignore ImGuiFocusedFlags;
%ignore ImGuiHoveredFlags;
%ignore ImGuiInputTextFlags;
%ignore ImGuiSelectableFlags;
%ignore ImGuiTabBarFlags;
%ignore ImGuiTabItemFlags;
%ignore ImGuiTreeNodeFlags;
%ignore ImGuiViewportFlags;
%ignore ImGuiWindowFlags;

%rename("%(camelcase)s", %$isenum) "";
%rename("%(camelcase)s", %$isenumitem) "";
%rename("%(camelcase)s", %$isvariable, %$ispublic) "";

%apply void* VOID_INT_PTR {
    ImFont*,
    ImGuiSizeCallback,
    ImGuiInputTextCallback,
    ImGuiSizeConstraintCallbackData*,
    ImGuiInputTextCallbackData*,
    ImGuiSizeCallbackData*
}

%ignore ImGuiSizeConstraintCallbackData;
%ignore ImGuiInputTextCallbackData;
%ignore ImGuiSizeCallbackData;

%ignore ImGui::SetAllocatorFunctions;
%ignore ImGui::SaveIniSettingsToMemory;
%ignore ImGui::LogTextV;
%ignore ImGui::TextV;
%ignore ImGui::TextColoredV;
%ignore ImGui::TextDisabledV;
%ignore ImGui::TextWrappedV;
%ignore ImGui::LabelTextV;
%ignore ImGui::BulletTextV;
%ignore ImGui::TreeNodeV;
%ignore ImGui::TreeNodeExV;
%ignore ImGui::SetTooltipV;
%ignore ImGui::ColorConvertRGBtoHSV;
%ignore ImGui::SetNextWindowSizeConstraints(const ImVec2& size_min, const ImVec2& size_max, ImGuiSizeConstraintCallback custom_callback, void* custom_callback_data = NULL);
%ignore ImGui::PlotLines(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0));
%ignore ImGui::PlotHistogram(const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0,0));
%ignore ImGui::Combo(const char* label, int* current_item, bool(*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int popup_max_height_in_items = -1);
%ignore ImGui::ListBox(const char* label, int* current_item, bool (*items_getter)(void* data, int idx, const char** out_text), void* data, int items_count, int height_in_items = -1);
%ignore ImGuiTextBuffer;
%ignore ImGuiStorage;
%ignore ImGuiViewport;
%ignore ImGui::SetStateStorage;
%ignore ImGui::GetStateStorage;
%ignore ImGui::GetPlatformIO;
%ignore ImGui::GetWindowViewport;
%ignore ImGui::DockSpaceOverViewport;
%ignore ImGui::GetMainViewport;
%ignore ImGui::FindViewportByID;
%ignore ImGui::FindViewportByPlatformHandle;

%ignore ImGui::PlotEx;
%ignore ImGui::ImFontAtlasBuildMultiplyCalcLookupTable;
%ignore ImGui::ImFontAtlasBuildMultiplyRectAlpha8;

%ignore ImTextStrToUtf8;
%ignore ImTextCharFromUtf8;
%ignore ImTextStrFromUtf8;
%ignore ImTextCountCharsFromUtf8;
%ignore ImTextCountUtf8BytesFromStr;
%ignore ImFileOpen;
%ignore ImFileLoadToMemory;
%ignore ImStrlenW;
%ignore ImStrbolW;
%ignore ImStristr;
%ignore ImFormatString;
%ignore ImFormatStringV;
%ignore ImSwap;
%ignore ImDrawList;
%ignore ImDrawListSplitter;
%ignore ImGuiPlatformIO;
%ignore ImFontGlyphRangesBuilder;

%ignore ImDrawChannel;
%ignore ImDrawCmd;
%ignore ImDrawData;
%ignore ImDrawList;
%ignore ImDrawVert;
%ignore ImFont;
%ignore ImFontAtlas;
%ignore ImFontConfig;
%ignore ImFontGlyph;
%ignore ImGuiIO;
%ignore ImGuiTextFilter;
%ignore ImGuiPayload;
%ignore ImGui::CreateContext;
%ignore ImGui::DestroyContext;
%ignore ImGui::GetCurrentContext;
%ignore ImGui::SetCurrentContext;
%ignore ImGui::GetDrawListSharedData;
%ignore ImGui::GetBackgroundDrawList;
%ignore ImGui::GetForegroundDrawList;
%ignore ImGui::GetDrawData;
%ignore ImGui::GetIO;
%ignore ImGui::GetWindowDrawList;
%ignore ImGui::AcceptDragDropPayload;
%ignore ImGuiStyle::Colors;
%ignore ImGui::GetDragDropPayload;
%ignore ImGui::GetDragDropPayload;

%ignore ImGui::ShowDemoWindow;
%ignore ImGui::ShowAboutWindow;
%ignore ImGui::ShowStyleEditor;
%ignore ImGui::ShowStyleSelector;
%ignore ImGui::ShowFontSelector;
%ignore ImGui::ShowUserGuide;

%define %imgui_enum(NAME)
    %ignore NAME;
    %rename(NAME) NAME##_;
    %typemap(cstype) NAME "NAME";
    %typemap(csin) NAME "(int)$csinput"
    %typemap(csout, excode=SWIGEXCODE) NAME {
        var res = $imcall;$excode
        return ($typemap(cstype, NAME))res;
    }
    %typemap(csvarout) NAME "get $typemap(csout, NAME)"
%enddef

%ignore ImGuiColorEditFlags_DefaultOptions_;
%ignore ImGuiColorEditFlags_DisplayMask_;
%ignore ImGuiColorEditFlags_DataTypeMask_;
%ignore ImGuiColorEditFlags_PickerMask_;
%ignore ImGuiColorEditFlags_InputMask_;

%imgui_enum(ImDrawCornerFlags);
%imgui_enum(ImGuiCol);
%imgui_enum(ImGuiComboFlags);
%imgui_enum(ImGuiDragDropFlags);
%imgui_enum(ImGuiHoveredFlags);
%imgui_enum(ImGuiKey);
%imgui_enum(ImGuiSelectableFlags);
%imgui_enum(ImGuiTreeNodeFlags);
%imgui_enum(ImDrawListFlags);
%imgui_enum(ImGuiColorEditFlags);
%imgui_enum(ImGuiCond);
%imgui_enum(ImGuiFocusedFlags);
%imgui_enum(ImGuiInputTextFlags);
%imgui_enum(ImGuiMouseCursor);
%imgui_enum(ImGuiStyleVar);
%imgui_enum(ImGuiWindowFlags);
%imgui_enum(ImFontAtlasFlags);
%imgui_enum(ImGuiBackendFlags);
%imgui_enum(ImGuiConfigFlags);
%imgui_enum(ImGuiDataType);
%imgui_enum(ImGuiDir);
%imgui_enum(ImGuiNavInput);
%imgui_enum(ImGuiMouseButton);

URHO3D_BINARY_COMPATIBLE_TYPE_EX(Urho3DNet.Vector2, ImVec2, pod::float2);
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Urho3DNet.Color, ImVec4, pod::float4);
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Urho3DNet.Color, ImColor, pod::float4);

%include "../include/ImGui/imgui.h"
//%include "../include/ImGui/imgui_internal.h"
