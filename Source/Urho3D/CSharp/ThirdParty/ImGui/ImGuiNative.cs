using System.Runtime.InteropServices;
using System.Numerics;
using System;
using System.Security;

namespace ImGui
{
    /// <summary>
    /// Contains all of the exported functions from the native (c)imGui module.
    /// </summary>
    public static unsafe class ImGuiNative
    {
        private const string cimguiLib = Urho3D.CSharp.Config.NativeLibraryName;

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeIO* igGetIO();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeStyle* igGetStyle();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern DrawData* igGetDrawData();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igNewFrame();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igNewLine();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igRender();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igShutdown();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igShowUserGuide();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igShowStyleEditor(ref NativeStyle @ref);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igShowDemoWindow(ref bool opened);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igShowMetricsWindow(ref bool opened);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igShowStyleSelector(string label);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igShowFontSelector(string label);


        // Window
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBegin(string name, byte* p_opened, WindowFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBegin2(string name, byte* p_opened, Vector2 size_on_first_use, float bg_alpha, WindowFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBegin(string name, ref bool p_opened, WindowFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBegin2(string name, ref bool p_opened, Vector2 size_on_first_use, float bg_alpha, WindowFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEnd();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginChild(string str_id, Vector2 size, bool border, WindowFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginChildEx(uint id, Vector2 size, bool border, WindowFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndChild();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetContentRegionMax(out Vector2 @out);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetContentRegionAvail(out Vector2 @out);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetContentRegionAvailWidth();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetWindowContentRegionMin(out Vector2 @out);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetWindowContentRegionMax(out Vector2 @out);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetWindowContentRegionWidth();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeDrawList* igGetWindowDrawList();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsWindowAppearing();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowFontScale(float scale);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetWindowPos(out Vector2 @out);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetWindowSize(out Vector2 @out);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetWindowWidth();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetWindowHeight();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsWindowCollapsed();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetNextWindowPos(Vector2 pos, Condition cond, Vector2 pivot);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetNextWindowSize(Vector2 size, Condition cond);
        public delegate void ImGuiSizeConstraintCallback(IntPtr data);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetNextWindowSizeConstraints(Vector2 size_min, Vector2 size_max, ImGuiSizeConstraintCallback custom_callback, void* custom_callback_data);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetNextWindowContentSize(Vector2 size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetNextWindowCollapsed(bool collapsed, Condition cond);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetNextWindowFocus();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowPos(Vector2 pos, Condition cond);  //(not recommended)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowSize(Vector2 size, Condition cond); //(not recommended)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowCollapsed(bool collapsed, Condition cond); //(not recommended)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowFocus(); //(not recommended)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowPosByName(string name, Vector2 pos, Condition cond);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowSize2(string name, Vector2 size, Condition cond);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowCollapsed2(string name, bool collapsed, Condition cond);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetWindowFocus2(string name);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetScrollX();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetScrollY();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetScrollMaxX();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetScrollMaxY();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetScrollX(float scroll_x);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetScrollY(float scroll_y);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetScrollHere(float center_y_ratio = 0.5f);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetScrollFromPosY(float pos_y, float center_y_ratio = 0.5f);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetItemDefaultFocus();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetKeyboardFocusHere(int offset);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetStateStorage(ref Storage tree);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern Storage* igGetStateStorage();

        // Parameters stacks (shared)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushFont(NativeFont* font);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopFont();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushStyleColor(ColorTarget idx, Vector4 col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopStyleColor(int count);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushStyleVar(StyleVar idx, float val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushStyleVarVec(StyleVar idx, Vector2 val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopStyleVar(int count);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeFont* igGetFont();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetFontSize();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetFontTexUvWhitePixel(Vector2* pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint igGetColorU32(ColorTarget idx, float alpha_mul);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint igGetColorU32Vec(Vector4* col);

        // Parameters stacks (current window)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushItemWidth(float item_width);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopItemWidth();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igCalcItemWidth();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushTextWrapPos(float wrap_pos_x);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopTextWrapPos();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushAllowKeyboardFocus(bool v);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopAllowKeyboardFocus();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushButtonRepeat(bool repeat);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopButtonRepeat();

        // Layout
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSeparator();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSameLine(float local_pos_x, float spacing_w);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSpacing();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igDummy(Vector2* size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igIndent(float indent_w = 0.0f);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igUnindent(float indent_w = 0.0f);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igBeginGroup();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndGroup();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetCursorPos(Vector2* pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetCursorPosX();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetCursorPosY();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetCursorPos(Vector2 local_pos);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetCursorPosX(float x);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetCursorPosY(float y);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetCursorStartPos(out Vector2 pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetCursorScreenPos(Vector2* pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetCursorScreenPos(Vector2 pos);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igAlignTextToFramePadding();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetTextLineHeight();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetTextLineHeightWithSpacing();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetFrameHeight();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetFrameHeightWithSpacing();

        // Columns
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igColumns(int count, string id, bool border);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igNextColumn();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int igGetColumnIndex();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetColumnOffset(int column_index);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetColumnOffset(int column_index, float offset_x);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetColumnWidth(int column_index);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetColumnWidth(int column_index, float width);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int igGetColumnsCount();


        // ID scopes
        // If you are creating widgets in a loop you most likely want to push a unique identifier so ImGui can differentiate them
        // You can also use "##extra" within your widget name to distinguish them from each others (see 'Programmer Guide')
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushIDStr(string str_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushIDStrRange(string str_begin, string str_end);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushIDPtr(void* ptr_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushIDInt(int int_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopID();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint igGetIDStr(string str_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint igGetIDStrRange(string str_begin, string str_end);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint igGetIDPtr(void* ptr_id);

        // Widgets
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igText(string fmt);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTextColored(Vector4 col, string fmt);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTextDisabled(string fmt);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTextWrapped(string fmt);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTextUnformatted(byte* text, byte* text_end);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igLabelText(string label, string fmt);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igBullet();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igBulletText(string fmt);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igButton(string label, Vector2 size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSmallButton(string label);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInvisibleButton(string str_id, Vector2 size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igImage(IntPtr user_texture_id, Vector2 size, Vector2 uv0, Vector2 uv1, Vector4 tint_col, Vector4 border_col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igImageButton(IntPtr user_texture_id, Vector2 size, Vector2 uv0, Vector2 uv1, int frame_padding, Vector4 bg_col, Vector4 tint_col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igCheckbox(string label, ref bool v);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igCheckboxFlags(string label, UIntPtr* flags, uint flags_value);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igRadioButtonBool(string label, bool active);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igRadioButton(string label, int* v, int v_button);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginCombo(string label, string preview_value, ComboFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndCombo();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igCombo(string label, ref int current_item, string[] items, int items_count, int height_in_items);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igCombo2(string label, ref int current_item, string items_separated_by_zeros, int height_in_items);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igCombo3(string label, ref int current_item, ItemSelectedCallback items_getter, IntPtr data, int items_count, int height_in_items);

        public delegate IntPtr ImGuiContextAllocationFunction(UIntPtr size);
        public delegate void ImGuiContextFreeFunction(IntPtr ptr);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeContext* igCreateContext(ImGuiContextAllocationFunction malloc_fn, ImGuiContextFreeFunction free_fn);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igDestroyContext(NativeContext* ctx);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeContext* igGetCurrentContext();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetCurrentContext(NativeContext* ctx);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igColorButton(string desc_id, Vector4 col, ColorEditFlags flags, Vector2 size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igColorEdit3(string label, Vector3* col, ColorEditFlags flags = 0);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igColorEdit4(string label, Vector4* col, ColorEditFlags flags = 0);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igColorPicker3(string label, Vector3* col, ColorEditFlags flags = 0);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igColorPicker4(string label, Vector4* col, ColorEditFlags flags = 0, float* ref_col = null);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void SetColorEditOptions(ColorEditFlags flags);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPlotLines(string label, float* values, int values_count, int values_offset, string overlay_text, float scale_min, float scale_max, Vector2 graph_size, int stride);
        public delegate float ImGuiPlotHistogramValuesGetter(IntPtr data, int idx);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPlotLines2(string label, ImGuiPlotHistogramValuesGetter values_getter, void* data, int values_count, int values_offset, string overlay_text, float scale_min, float scale_max, Vector2 graph_size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPlotHistogram(string label, float* values, int values_count, int values_offset, string overlay_text, float scale_min, float scale_max, Vector2 graph_size, int stride);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPlotHistogram2(string label, ImGuiPlotHistogramValuesGetter values_getter, void* data, int values_count, int values_offset, string overlay_text, float scale_min, float scale_max, Vector2 graph_size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igProgressBar(float fraction, Vector2* size_arg, string overlay);
        // Widgets: Sliders (tip: ctrl+click on a slider to input text)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderFloat(string label, float* v, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderFloat(string label, ref float v, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderFloat2(string label, ref Vector2 v, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderFloat3(string label, ref Vector3 v, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderFloat4(string label, ref Vector4 v, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderAngle(string label, ref float v_rad, float v_degrees_min, float v_degrees_max);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderInt(string label, ref int v, int v_min, int v_max, string display_format);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderInt2(string label, ref Int2 v, int v_min, int v_max, string display_format);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderInt3(string label, ref Int3 v, int v_min, int v_max, string display_format);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSliderInt4(string label, ref Int4 v, int v_min, int v_max, string display_format);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igVSliderFloat(string label, Vector2 size, float* v, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igVSliderInt(string label, Vector2 size, int* v, int v_min, int v_max, string display_format);

        // Widgets: Drags (tip: ctrl+click on a drag box to input text)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragFloat(string label, ref float v, float v_speed, float v_min, float v_max, string display_format, float power);     // If v_max >= v_max we have no bound
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragFloat2(string label, ref Vector2 v, float v_speed, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragFloat3(string label, ref Vector3 v, float v_speed, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragFloat4(string label, ref Vector4 v, float v_speed, float v_min, float v_max, string display_format, float power);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragFloatRange2(string label, ref float v_current_min, ref float v_current_max, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, string display_format = "%.3f", string display_format_max = null, float power = 1.0f);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragInt(string label, ref int v, float v_speed, int v_min, int v_max, string display_format);                                       // If v_max >= v_max we have no bound
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragInt2(string label, ref Int2 v, float v_speed, int v_min, int v_max, string display_format);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragInt3(string label, ref Int3 v, float v_speed, int v_min, int v_max, string display_format);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragInt4(string label, ref Int4 v, float v_speed, int v_min, int v_max, string display_format);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igDragIntRange2(string label, ref int v_current_min, ref int v_current_max, float v_speed = 1.0f, int v_min = 0, int v_max = 0, string display_format = "%.0f", string display_format_max = null);


        // Widgets: Input
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputText(string label, IntPtr buffer, uint buf_size, InputTextFlags flags, TextEditCallback callback, void* user_data);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputTextMultiline(string label, IntPtr buffer, uint buf_size, Vector2 size, InputTextFlags flags, TextEditCallback callback, void* user_data);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputFloat(string label, float* v, float step, float step_fast, int decimal_precision, InputTextFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputFloat2(string label, Vector2 v, int decimal_precision, InputTextFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputFloat3(string label, Vector3 v, int decimal_precision, InputTextFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputFloat4(string label, Vector4 v, int decimal_precision, InputTextFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputInt(string label, int* v, int step, int step_fast, InputTextFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputInt2(string label, Int2 v, InputTextFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputInt3(string label, Int3 v, InputTextFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igInputInt4(string label, Int4 v, InputTextFlags extra_flags);

        // Widgets: Trees
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igTreeNode(string str_label_id);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igTreeNodeEx(string label, TreeNodeFlags flags = 0);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igTreeNodeStr(string str_id, string fmt);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igTreeNodePtr(void* ptr_id, string fmt);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTreePushStr(string str_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTreePushPtr(void* ptr_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTreePop();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igTreeAdvanceToLabelPos();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetTreeNodeToLabelSpacing();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetNextTreeNodeOpen(bool opened, Condition cond);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igCollapsingHeader(string label, TreeNodeFlags flags = 0);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igCollapsingHeaderEx(string label, ref bool p_open, TreeNodeFlags flags = 0);

        // Widgets: Selectable / Lists
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSelectable(string label, bool selected, SelectableFlags flags, Vector2 size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSelectableEx(string label, ref bool p_selected, SelectableFlags flags, Vector2 size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igListBox(string label, int* current_item, char** items, int items_count, int height_in_items);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igListBox2(string label, ref int currentItem, ItemSelectedCallback items_getter, IntPtr data, int items_count, int height_in_items);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igListBoxHeader(string label, Vector2 size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igListBoxHeader2(string label, int items_count, int height_in_items);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igListBoxFooter();

        // Widgets: Value() Helpers. Output single value in "name: value" format (tip: freely declare your own within the ImGui namespace!)
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igValueBool(string prefix, bool b);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igValueInt(string prefix, int v);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igValueUInt(string prefix, uint v);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igValueFloat(string prefix, float v, string float_format);

        // Tooltip
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetTooltip(string fmt);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igBeginTooltip();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndTooltip();

        // Widgets: Menus
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginMainMenuBar();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndMainMenuBar();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginMenuBar();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndMenuBar();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginMenu(string label, bool enabled);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndMenu();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igMenuItem(string label, string shortcut, bool selected, bool enabled);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igMenuItemPtr(string label, string shortcut, bool* p_selected, bool enabled);

        // Popup
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igOpenPopup(string str_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igOpenPopupOnItemClick(string str_id, int mouse_button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginPopup(string str_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginPopupModal(string name, byte* p_opened, WindowFlags extra_flags);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginPopupContextItem(string str_id, int mouse_button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginPopupContextWindow(string str_id, int mouse_button, bool also_over_items);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginPopupContextVoid(string str_id, int mouse_button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndPopup();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool igIsPopupOpen(string str_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igCloseCurrentPopup();

        // Logging: all text output from interface is redirected to tty/file/clipboard. Tree nodes are automatically opened.
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igLogToTTY(int max_depth);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igLogToFile(int max_depth, string filename);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igLogToClipboard(int max_depth);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igLogFinish();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igLogButtons();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        //public static extern void igLogText(string fmt, ...);
        public static extern void igLogText(string fmt);

        // Drag-drop

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginDragDropSource(DragDropFlags flags, int mouse_button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igSetDragDropPayload(string type, void* data, uint size, Condition cond);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndDragDropSource();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginDragDropTarget();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativePayload* igAcceptDragDropPayload(string type, DragDropFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndDragDropTarget();

        // Clipping
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPushClipRect(Vector2 clip_rect_min, Vector2 clip_rect_max, byte intersect_with_current_clip_rect);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igPopClipRect();

        // Built-in Styles
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igStyleColorsClassic(NativeStyle* dst);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igStyleColorsDark(NativeStyle* dst);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igStyleColorsLight(NativeStyle* dst);

        // Utilities
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsItemHovered(HoveredFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsItemActive();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsItemClicked(int mouse_button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsItemVisible();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsAnyItemHovered();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsAnyItemActive();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetItemRectMin(out Vector2 pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetItemRectMax(out Vector2 pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetItemRectSize(out Vector2 pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetItemAllowOverlap();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsWindowHovered(HoveredFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsWindowFocused(FocusedFlags flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsRectVisible(Vector2 item_size);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsRectVisible2(Vector2* rect_min, Vector2* rect_max);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float igGetTime();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int igGetFrameCount();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeDrawList* igGetOverlayDrawList();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern string igGetStyleColorName(ColorTarget idx);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igCalcItemRectClosestPoint(out Vector2 pOut, Vector2 pos, bool on_edge, float outward);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igCalcTextSize(out Vector2 pOut, char* text, char* text_end, [MarshalAs(UnmanagedType.I1)] bool hide_text_after_double_hash, float wrap_width);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igCalcListClipping(int items_count, float items_height, ref int out_items_display_start, ref int out_items_display_end);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igBeginChildFrame(uint id, Vector2 size, WindowFlags extra_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igEndChildFrame();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igColorConvertU32ToFloat4(Vector4* pOut, uint @in);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern uint igColorConvertFloat4ToU32(Vector4 @in);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igColorConvertRGBtoHSV(float r, float g, float b, float* out_h, float* out_s, float* out_v);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igColorConvertHSVtoRGB(float h, float s, float v, float* out_r, float* out_g, float* out_b);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int igGetKeyIndex(int imgui_key);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsKeyDown(int user_key_index);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsKeyPressed(int user_key_index, bool repeat);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsKeyReleased(int user_key_index);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int igGetKeyPressedAmount(int key_index, float repeat_delay, float rate);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsMouseDown(int button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsMouseClicked(int button, [MarshalAs(UnmanagedType.I1)] bool repeat);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsMouseDoubleClicked(int button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsMouseReleased(int button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsAnyWindowHovered();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsMousePosValid(Vector2* mousePos);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsMouseHoveringRect(Vector2 pos_min, Vector2 pos_max, bool clip);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool igIsMouseDragging(int button, float lock_threshold);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetMousePos(out Vector2 pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetMousePosOnOpeningCurrentPopup(out Vector2 pOut);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igGetMouseDragDelta(out Vector2 pOut, int button, float lock_threshold);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igResetMouseDragDelta(int button);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern MouseCursorKind igGetMouseCursor();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetMouseCursor(MouseCursorKind type);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igCaptureKeyboardFromApp();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igCaptureMouseFromApp();

        // Helpers functions to access functions pointers @in ImGui::GetIO()
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void* igMemAlloc(uint sz);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igMemFree(void* ptr);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern string igGetClipboardText();
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void igSetClipboardText(string text);

        // public state access - if you want to share ImGui state between modules (e.g. DLL) or allocate it yourself
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern string igGetVersion();
        /*
        CIMGUI_API struct ImGuiContext*    igCreateContext(void* (*malloc_fn)(size_t), void (*free_fn)(void*));
        CIMGUI_API void                    igDestroyContext(struct ImGuiContext* ctx);
        CIMGUI_API struct ImGuiContext*    igGetCurrentContext();
        CIMGUI_API void                    igSetCurrentContext(struct ImGuiContext* ctx);
        */

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImFontConfig_DefaultConstructor(FontConfig* config);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImFontAtlas_GetTexDataAsRGBA32(NativeFontAtlas* atlas, byte** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImFontAtlas_GetTexDataAsAlpha8(NativeFontAtlas* atlas, byte** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImFontAtlas_SetTexID(NativeFontAtlas* atlas, void* id);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeFont* ImFontAtlas_AddFont(NativeFontAtlas* atlas, ref FontConfig font_cfg);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeFont* ImFontAtlas_AddFontDefault(NativeFontAtlas* atlas, IntPtr font_cfg);
        public static NativeFont* ImFontAtlas_AddFontDefault(NativeFontAtlas* atlas) { return ImFontAtlas_AddFontDefault(atlas, IntPtr.Zero); }

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeFont* ImFontAtlas_AddFontFromFileTTF(NativeFontAtlas* atlas, string filename, float size_pixels, IntPtr font_cfg, char* glyph_ranges);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeFont* ImFontAtlas_AddFontFromMemoryTTF(NativeFontAtlas* atlas, void* ttf_data, int ttf_size, float size_pixels, IntPtr font_cfg, char* glyph_ranges);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeFont* ImFontAtlas_AddFontFromMemoryCompressedTTF(NativeFontAtlas* atlas, void* compressed_ttf_data, int compressed_ttf_size, float size_pixels, FontConfig* font_cfg, char* glyph_ranges);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern NativeFont* ImFontAtlas_AddFontFromMemoryCompressedBase85TTF(NativeFontAtlas* atlas, string compressed_ttf_data_base85, float size_pixels, FontConfig* font_cfg, char* glyph_ranges);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImFontAtlas_ClearTexData(NativeFontAtlas* atlas);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImFontAtlas_Clear(NativeFontAtlas* atlas);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern char* ImFontAtlas_GetGlyphRangesDefault(NativeFontAtlas* atlas);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern char* ImFontAtlas_GetGlyphRangesKorean(NativeFontAtlas* atlas);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern char* ImFontAtlas_GetGlyphRangesJapanese(NativeFontAtlas* atlas);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern char* ImFontAtlas_GetGlyphRangesChinese(NativeFontAtlas* atlas);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern char* ImFontAtlas_GetGlyphRangesCyrillic(NativeFontAtlas* atlas);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern char* ImFontAtlas_GetGlyphRangesThai(NativeFontAtlas* atlas);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiIO_AddInputCharacter(ushort c);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiIO_AddInputCharactersUTF8(string utf8_chars);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiIO_ClearInputCharacters();

        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ImDrawList_GetVertexBufferSize(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern DrawVert* ImDrawList_GetVertexPtr(NativeDrawList* list, int n);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ImDrawList_GetIndexBufferSize(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern ushort* ImDrawList_GetIndexPtr(NativeDrawList* list, int n);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ImDrawList_GetCmdSize(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern DrawCmd* ImDrawList_GetCmdPtr(NativeDrawList* list, int n);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawData_DeIndexAllBuffers(DrawData* drawData);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawData_ScaleClipRects(DrawData* drawData, Vector2 sc);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_Clear(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_ClearFreeMemory(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PushClipRect(NativeDrawList* list, Vector2 clip_rect_min, Vector2 clip_rect_max, byte intersect_with_current_clip_rect);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PushClipRectFullScreen(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PopClipRect(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PushTextureID(NativeDrawList* list, void* texture_id);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PopTextureID(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddLine(NativeDrawList* list, Vector2 a, Vector2 b, uint col, float thickness);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddRect(NativeDrawList* list, Vector2 a, Vector2 b, uint col, float rounding, int rounding_corners_flags, float thickness);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddRectFilled(NativeDrawList* list, Vector2 a, Vector2 b, uint col, float rounding, int rounding_corners_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddRectFilledMultiColor(NativeDrawList* list, Vector2 a, Vector2 b, uint col_upr_left, uint col_upr_right, uint col_bot_right, uint col_bot_left);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddQuad(NativeDrawList* list, Vector2 a, Vector2 b, Vector2 c, Vector2 d, uint col, float thickness);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddQuadFilled(NativeDrawList* list, Vector2 a, Vector2 b, Vector2 c, Vector2 d, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddTriangle(NativeDrawList* list, Vector2 a, Vector2 b, Vector2 c, uint col, float thickness);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddTriangleFilled(NativeDrawList* list, Vector2 a, Vector2 b, Vector2 c, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddCircle(NativeDrawList* list, Vector2 centre, float radius, uint col, int num_segments, float thickness);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddCircleFilled(NativeDrawList* list, Vector2 centre, float radius, uint col, int num_segments);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddText(NativeDrawList* list, Vector2 pos, uint col, byte* text_begin, byte* text_end);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddTextExt(NativeDrawList* list, NativeFont* font, float font_size, Vector2 pos, uint col, byte* text_begin, byte* text_end, float wrap_width, Vector4* cpu_fine_clip_rect);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddImage(NativeDrawList* list, void* user_texture_id, Vector2 a, Vector2 b, Vector2 uv_a, Vector2 uv_b, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddImageQuad(NativeDrawList* list, void* user_texture_id, Vector2 a, Vector2 b, Vector2 c, Vector2 d, Vector2 uv_a, Vector2 uv_b, Vector2 uv_c, Vector2 uv_d, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddImageRounded(NativeDrawList* list, void* user_texture_id, Vector2 a, Vector2 b, Vector2 uv_a, Vector2 uv_b, uint col, float rounding, int rounding_corners);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddPolyline(NativeDrawList* list, Vector2* points, int num_points, uint col, byte closed, float thickness);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddConvexPolyFilled(NativeDrawList* list, Vector2* points, int num_points, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddBezierCurve(NativeDrawList* list, Vector2 pos0, Vector2 cp0, Vector2 cp1, Vector2 pos1, uint col, float thickness, int num_segments);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathClear(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathLineTo(NativeDrawList* list, Vector2 pos);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathLineToMergeDuplicate(NativeDrawList* list, Vector2 pos);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathFillConvex(NativeDrawList* list, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathStroke(NativeDrawList* list, uint col, byte closed, float thickness);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathArcTo(NativeDrawList* list, Vector2 centre, float radius, float a_min, float a_max, int num_segments);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathArcToFast(NativeDrawList* list, Vector2 centre, float radius, int a_min_of_12, int a_max_of_12);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathBezierCurveTo(NativeDrawList* list, Vector2 p1, Vector2 p2, Vector2 p3, int num_segments);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PathRect(NativeDrawList* list, Vector2 rect_min, Vector2 rect_max, float rounding, int rounding_corners_flags);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_ChannelsSplit(NativeDrawList* list, int channels_count);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_ChannelsMerge(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_ChannelsSetCurrent(NativeDrawList* list, int channel_index);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddCallback(NativeDrawList* list, ImDrawCallback callback, void* callback_data);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_AddDrawCmd(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PrimReserve(NativeDrawList* list, int idx_count, int vtx_count);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PrimRect(NativeDrawList* list, Vector2 a, Vector2 b, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PrimRectUV(NativeDrawList* list, Vector2 a, Vector2 b, Vector2 uv_a, Vector2 uv_b, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PrimQuadUV(NativeDrawList* list, Vector2 a, Vector2 b, Vector2 c, Vector2 d, Vector2 uv_a, Vector2 uv_b, Vector2 uv_c, Vector2 uv_d, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PrimVtx(NativeDrawList* list, Vector2 pos, Vector2 uv, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PrimWriteVtx(NativeDrawList* list, Vector2 pos, Vector2 uv, uint col);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_PrimWriteIdx(NativeDrawList* list, ushort idx);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_UpdateClipRect(NativeDrawList* list);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImDrawList_UpdateTextureID(NativeDrawList* list);

        // List Clipper
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiListClipper_Begin(void* clipper, int count, float items_height);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiListClipper_End(void* clipper);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool ImGuiListClipper_Step(void* clipper);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ImGuiListClipper_GetDisplayStart(void* clipper);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ImGuiListClipper_GetDisplayEnd(void* clipper);

        // ImGuiTextFilter
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiTextFilter_Init(void* filter, char* default_filter);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiTextFilter_Clear(void* filter);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ImGuiTextFilter_Draw(void* filter, char* label, float width);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ImGuiTextFilter_PassFilter(void* filter, char* text, char* text_end);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ImGuiTextFilter_IsActive(void* filter);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiTextFilter_Build(void* filter);

        // ImGuiTextEditCallbackData
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiTextEditCallbackData_DeleteChars(void* data, int pos, int bytes_count);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiTextEditCallbackData_InsertChars(void* data, int pos, char* text, char* text_end);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ImGuiTextEditCallbackData_HasSelection(void* data);

        // ImGuiStorage
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiStorage_Init(void* storage);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiStorage_Clear(void* storage);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int ImGuiStorage_GetInt(void* storage, uint key, int default_val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiStorage_SetInt(void* storage, uint key, int val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool ImGuiStorage_GetBool(void* storage, uint key, bool default_val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiStorage_SetBool(void* storage, uint key, bool val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float ImGuiStorage_GetFloat(void* storage, uint key, float default_val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiStorage_SetFloat(void* storage, uint key, float val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void* ImGuiStorage_GetVoidPtr(void* storage, uint key);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiStorage_SetVoidPtr(void* storage, uint key, void* val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern int* ImGuiStorage_GetIntRef(void* storage, uint key, int default_val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern bool* ImGuiStorage_GetBoolRef(void* storage, uint key, bool default_val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern float* ImGuiStorage_GetFloatRef(void* storage, uint key, float default_val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void** ImGuiStorage_GetVoidPtrRef(void* storage, uint key, void* default_val);
        [SuppressUnmanagedCodeSecurity]
        [DllImport(cimguiLib, CallingConvention = CallingConvention.Cdecl)]
        public static extern void ImGuiStorage_SetAllInt(void* storage, int val);
    }

    public delegate bool ItemSelectedCallback(IntPtr data, int index, string out_text);
    public unsafe delegate void ImDrawCallback(DrawList* parent_list, DrawCmd* cmd);
}
