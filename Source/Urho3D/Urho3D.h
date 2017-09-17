

#ifndef URHO3D_API_H
#define URHO3D_API_H

#ifdef _MSC_VER

#pragma warning(disable: 4251)
#pragma warning(disable: 4275)

#ifdef URHO3D_STATIC_DEFINE
#  define URHO3D_API
#  define URHO3D_NO_EXPORT
#else
#  ifndef URHO3D_API
#    ifdef URHO3D_EXPORTS
        /* We are building this library */
#      define URHO3D_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define URHO3D_API __declspec(dllimport)
#    endif
#  endif

#  ifndef URHO3D_NO_EXPORT
#    define URHO3D_NO_EXPORT 
#  endif
#endif

#ifndef URHO3D_DEPRECATED
#  define URHO3D_DEPRECATED __declspec(deprecated)
#endif

#ifndef URHO3D_DEPRECATED_EXPORT
#  define URHO3D_DEPRECATED_EXPORT URHO3D_API URHO3D_DEPRECATED
#endif

#ifndef URHO3D_DEPRECATED_NO_EXPORT
#  define URHO3D_DEPRECATED_NO_EXPORT URHO3D_NO_EXPORT URHO3D_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define URHO3D_NO_DEPRECATED
#endif

#else

#ifdef URHO3D_STATIC_DEFINE
#ifndef URHO3D_API
#  define URHO3D_API
#endif
#  define URHO3D_NO_EXPORT
#else
#  ifndef URHO3D_API
#    ifdef URHO3D_EXPORTS
/* We are building this library */
#      define URHO3D_API __attribute__((visibility("default")))
#    else
/* We are using this library */
#      define URHO3D_API __attribute__((visibility("default")))
#    endif
#  endif

#  ifndef URHO3D_NO_EXPORT
#    define URHO3D_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef URHO3D_DEPRECATED
#  define URHO3D_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef URHO3D_DEPRECATED_EXPORT
#  define URHO3D_DEPRECATED_EXPORT URHO3D_API URHO3D_DEPRECATED
#endif

#ifndef URHO3D_DEPRECATED_NO_EXPORT
#  define URHO3D_DEPRECATED_NO_EXPORT URHO3D_NO_EXPORT URHO3D_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define URHO3D_NO_DEPRECATED
#endif

#endif

#endif

