#include <Urho3D/Core/Main.h>
#include <Urho3D/Script/Script.h>

typedef int(SWIGSTDCALL* MainFunctionCallback)();
static MainFunctionCallback g_OverrideSDLMain = nullptr;

extern "C" void Urho3D_SetMainFunction(MainFunctionCallback callback)
{
    g_OverrideSDLMain = callback;
}

#if defined(__ANDROID__)
// Android C# applications need to provide a C# main function callback to SDL. This is achieved
// by decorating a main function with [SDLMain] attribute and adding a meta-data tag to AndroidManifest.xml.
// Meta-data entry: <meta-data android:name="mainLibrary" android:value="libUrho3D.so"></meta-data>
URHO3D_DEFINE_MAIN(g_OverrideSDLMain());
#endif
