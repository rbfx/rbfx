#pragma once

#include <Urho3D/Urho3D.h>

#ifndef PLUGIN_CORE_SAMPLEPLUGIN_API
    #ifdef URHO3D_STATIC
        #define PLUGIN_CORE_SAMPLEPLUGIN_API
    #else
        #if PLUGIN_CORE_SAMPLEPLUGIN_EXPORT
            #define PLUGIN_CORE_SAMPLEPLUGIN_API URHO3D_EXPORT_API
        #else
            #define PLUGIN_CORE_SAMPLEPLUGIN_API URHO3D_IMPORT_API
        #endif
    #endif
#endif
