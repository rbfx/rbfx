%module(naturalvar=1) ImGui

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1
%{

#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Rect.h>
#include <Urho3D/Math/Color.h>
#include <ImGui/imgui.h>
//#include <ImGui/imgui_internal.h>

%}

%include "typemaps.i"
%include "arrays_csharp.i"

%apply void* VOID_INT_PTR {
    void*,
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

// Speed boost
%pragma(csharp) imclassclassmodifiers="[System.Security.SuppressUnmanagedCodeSecurity]\ninternal class"
%typemap(csvarout, excode=SWIGEXCODE2) float INOUT[] "get { var ret = $imcall;$excode return ret; }"

%apply float *OUTPUT   { float& out_r, float& out_g, float& out_b, float& out_u, float& out_v, float& out_w };
%apply double *INOUT   { double* v };
%apply bool *INOUT     { bool*, unsigned int* flags };
%apply int INOUT[]     { int*, int[ANY] };
%apply float INOUT[]   { float*, float[ANY] };
%apply unsigned char OUTPUT[] { unsigned char out_table[256] };
%apply unsigned char INPUT[]  { unsigned char const table[256], unsigned char *pixels };
%apply bool INOUT[]    { bool[5] };

%typemap(ctype)   const char* INPUT[] "char**"
%typemap(cstype)  const char* INPUT[] "string[]"
%typemap(imtype, inattributes="[global::System.Runtime.InteropServices.In, global::System.Runtime.InteropServices.MarshalAs(global::System.Runtime.InteropServices.UnmanagedType.LPUTF8Str)]") const char* INPUT[] "string[]"
%typemap(csin)    const char* INPUT[] "$csinput"
%typemap(in)      const char* INPUT[] "$1 = $input;"
%typemap(freearg) const char* INPUT[] ""
%typemap(argout)  const char* INPUT[] ""

%apply const char* INPUT[]   { char const *const items[] };


//%apply unsigned int* INOUT { unsigned int* randomRef, unsigned int* nearestRef }

%ignore ImGui::SetAllocatorFunctions;
%ignore ImGui::SaveIniSettingsToMemory;
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

%include "../../../Urho3D/CSharp/Swig/Math.i"

URHO3D_BINARY_COMPATIBLE_TYPE_EX(Urho3DNet.Vector2, ImVec2, pod::float2);
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Urho3DNet.Color, ImVec4, pod::float4);
URHO3D_BINARY_COMPATIBLE_TYPE_EX(Urho3DNet.Color, ImColor, pod::float4);

%include "../include/ImGui/imgui.h"
//%include "../include/ImGui/imgui_internal.h"

