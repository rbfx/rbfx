/* -----------------------------------------------------------------------------
 * csharphead.swg
 *
 * Support code for exceptions if the SWIG_CSHARP_NO_EXCEPTION_HELPER is not defined
 * Support code for strings if the SWIG_CSHARP_NO_STRING_HELPER is not defined
 * ----------------------------------------------------------------------------- */

%insert(runtime) %{
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __cplusplus
#include <typeinfo> // rbfx
#endif
%}

%pragma(csharp) imclasscode=%{
  /// For Mono AOT platforms compatibility
  /// This attribute can be defined in any assembly for Mono AOT to use it.
  public class MonoPInvokeCallbackAttribute : global::System.Attribute {
    public MonoPInvokeCallbackAttribute(global::System.Type t) {
    }
  }
%}

#if !defined(SWIG_CSHARP_NO_EXCEPTION_HELPER)
%insert(runtime) %{
/* Support for throwing C# exceptions from C/C++. There are two types: 
 * Exceptions that take a message and ArgumentExceptions that take a message and a parameter name. */
typedef enum {
  SWIG_CSharpApplicationException,
  SWIG_CSharpArithmeticException,
  SWIG_CSharpDivideByZeroException,
  SWIG_CSharpIndexOutOfRangeException,
  SWIG_CSharpInvalidCastException,
  SWIG_CSharpInvalidOperationException,
  SWIG_CSharpIOException,
  SWIG_CSharpNullReferenceException,
  SWIG_CSharpOutOfMemoryException,
  SWIG_CSharpOverflowException,
  SWIG_CSharpSystemException
} SWIG_CSharpExceptionCodes;

typedef enum {
  SWIG_CSharpArgumentException,
  SWIG_CSharpArgumentNullException,
  SWIG_CSharpArgumentOutOfRangeException
} SWIG_CSharpExceptionArgumentCodes;

typedef void (SWIGSTDCALL* SWIG_CSharpExceptionCallback_t)(const char *);
typedef void (SWIGSTDCALL* SWIG_CSharpExceptionArgumentCallback_t)(const char *, const char *);

typedef struct {
  SWIG_CSharpExceptionCodes code;
  SWIG_CSharpExceptionCallback_t callback;
} SWIG_CSharpException_t;

typedef struct {
  SWIG_CSharpExceptionArgumentCodes code;
  SWIG_CSharpExceptionArgumentCallback_t callback;
} SWIG_CSharpExceptionArgument_t;

static SWIG_CSharpException_t SWIG_csharp_exceptions[] = {
  { SWIG_CSharpApplicationException, NULL },
  { SWIG_CSharpArithmeticException, NULL },
  { SWIG_CSharpDivideByZeroException, NULL },
  { SWIG_CSharpIndexOutOfRangeException, NULL },
  { SWIG_CSharpInvalidCastException, NULL },
  { SWIG_CSharpInvalidOperationException, NULL },
  { SWIG_CSharpIOException, NULL },
  { SWIG_CSharpNullReferenceException, NULL },
  { SWIG_CSharpOutOfMemoryException, NULL },
  { SWIG_CSharpOverflowException, NULL },
  { SWIG_CSharpSystemException, NULL }
};

static SWIG_CSharpExceptionArgument_t SWIG_csharp_exceptions_argument[] = {
  { SWIG_CSharpArgumentException, NULL },
  { SWIG_CSharpArgumentNullException, NULL },
  { SWIG_CSharpArgumentOutOfRangeException, NULL }
};

static void SWIGUNUSED SWIG_CSharpSetPendingException(SWIG_CSharpExceptionCodes code, const char *msg) {
  SWIG_CSharpExceptionCallback_t callback = SWIG_csharp_exceptions[SWIG_CSharpApplicationException].callback;
  if ((size_t)code < sizeof(SWIG_csharp_exceptions)/sizeof(SWIG_CSharpException_t)) {
    callback = SWIG_csharp_exceptions[code].callback;
  }
  callback(msg);
}

static void SWIGUNUSED SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpExceptionArgumentCodes code, const char *msg, const char *param_name) {
  SWIG_CSharpExceptionArgumentCallback_t callback = SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback;
  if ((size_t)code < sizeof(SWIG_csharp_exceptions_argument)/sizeof(SWIG_CSharpExceptionArgument_t)) {
    callback = SWIG_csharp_exceptions_argument[code].callback;
  }
  callback(msg, param_name);
}
%}

%insert(runtime) %{
#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterExceptionCallbacks_$module(
                                                SWIG_CSharpExceptionCallback_t applicationCallback,
                                                SWIG_CSharpExceptionCallback_t arithmeticCallback,
                                                SWIG_CSharpExceptionCallback_t divideByZeroCallback, 
                                                SWIG_CSharpExceptionCallback_t indexOutOfRangeCallback, 
                                                SWIG_CSharpExceptionCallback_t invalidCastCallback,
                                                SWIG_CSharpExceptionCallback_t invalidOperationCallback,
                                                SWIG_CSharpExceptionCallback_t ioCallback,
                                                SWIG_CSharpExceptionCallback_t nullReferenceCallback,
                                                SWIG_CSharpExceptionCallback_t outOfMemoryCallback, 
                                                SWIG_CSharpExceptionCallback_t overflowCallback, 
                                                SWIG_CSharpExceptionCallback_t systemCallback) {
  SWIG_csharp_exceptions[SWIG_CSharpApplicationException].callback = applicationCallback;
  SWIG_csharp_exceptions[SWIG_CSharpArithmeticException].callback = arithmeticCallback;
  SWIG_csharp_exceptions[SWIG_CSharpDivideByZeroException].callback = divideByZeroCallback;
  SWIG_csharp_exceptions[SWIG_CSharpIndexOutOfRangeException].callback = indexOutOfRangeCallback;
  SWIG_csharp_exceptions[SWIG_CSharpInvalidCastException].callback = invalidCastCallback;
  SWIG_csharp_exceptions[SWIG_CSharpInvalidOperationException].callback = invalidOperationCallback;
  SWIG_csharp_exceptions[SWIG_CSharpIOException].callback = ioCallback;
  SWIG_csharp_exceptions[SWIG_CSharpNullReferenceException].callback = nullReferenceCallback;
  SWIG_csharp_exceptions[SWIG_CSharpOutOfMemoryException].callback = outOfMemoryCallback;
  SWIG_csharp_exceptions[SWIG_CSharpOverflowException].callback = overflowCallback;
  SWIG_csharp_exceptions[SWIG_CSharpSystemException].callback = systemCallback;
}

#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterExceptionArgumentCallbacks_$module(
                                                SWIG_CSharpExceptionArgumentCallback_t argumentCallback,
                                                SWIG_CSharpExceptionArgumentCallback_t argumentNullCallback,
                                                SWIG_CSharpExceptionArgumentCallback_t argumentOutOfRangeCallback) {
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentException].callback = argumentCallback;
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentNullException].callback = argumentNullCallback;
  SWIG_csharp_exceptions_argument[SWIG_CSharpArgumentOutOfRangeException].callback = argumentOutOfRangeCallback;
}
%}

%pragma(csharp) imclasscode=%{
  public const string DllImportModule = "$dllimport_module";
#if NET47 || NETSTANDARD2_1 || NETCOREAPP3_0 || NET5_0
    public const global::System.Runtime.InteropServices.UnmanagedType LPStr = global::System.Runtime.InteropServices.UnmanagedType.LPUTF8Str;
#else
    public const global::System.Runtime.InteropServices.UnmanagedType LPStr = global::System.Runtime.InteropServices.UnmanagedType.LPStr;
#endif
  protected class SWIGExceptionHelper {

#if __IOS__
    [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
    public delegate void ExceptionDelegate(System.IntPtr message);
#if __IOS__
    [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
    public delegate void ExceptionArgumentDelegate(System.IntPtr message, System.IntPtr paramName);

    static delegate* unmanaged[Cdecl]<System.IntPtr, void> applicationDelegate = &SetPendingApplicationException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> arithmeticDelegate = &SetPendingArithmeticException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> divideByZeroDelegate = &SetPendingDivideByZeroException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> indexOutOfRangeDelegate = &SetPendingIndexOutOfRangeException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> invalidCastDelegate = &SetPendingInvalidCastException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> invalidOperationDelegate = &SetPendingInvalidOperationException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> ioDelegate = &SetPendingIOException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> nullReferenceDelegate = &SetPendingNullReferenceException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> outOfMemoryDelegate = &SetPendingOutOfMemoryException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> overflowDelegate = &SetPendingOverflowException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, void> systemDelegate = &SetPendingSystemException;

    static delegate* unmanaged[Cdecl]<System.IntPtr, System.IntPtr, void> argumentDelegate = &SetPendingArgumentException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, System.IntPtr, void> argumentNullDelegate = &SetPendingArgumentNullException;
    static delegate* unmanaged[Cdecl]<System.IntPtr, System.IntPtr, void> argumentOutOfRangeDelegate = &SetPendingArgumentOutOfRangeException;

    [global::System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="SWIGRegisterExceptionCallbacks_$module")]
    public static extern void SWIGRegisterExceptionCallbacks_$module(
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> applicationDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> arithmeticDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> divideByZeroDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> indexOutOfRangeDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> invalidCastDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> invalidOperationDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> ioDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> nullReferenceDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> outOfMemoryDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> overflowDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, void> systemExceptionDelegate);

    [global::System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="SWIGRegisterExceptionArgumentCallbacks_$module")]
    public static extern void SWIGRegisterExceptionCallbacksArgument_$module(
                                delegate* unmanaged[Cdecl]<System.IntPtr, System.IntPtr, void> argumentDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, System.IntPtr, void> argumentNullDelegate,
                                delegate* unmanaged[Cdecl]<System.IntPtr, System.IntPtr, void> argumentOutOfRangeDelegate);

    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingApplicationException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.ApplicationException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingArithmeticException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.ArithmeticException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingDivideByZeroException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.DivideByZeroException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingIndexOutOfRangeException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.IndexOutOfRangeException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingInvalidCastException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.InvalidCastException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingInvalidOperationException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.InvalidOperationException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingIOException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.IO.IOException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingNullReferenceException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.NullReferenceException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingOutOfMemoryException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.OutOfMemoryException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingOverflowException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.OverflowException(message, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingSystemException(System.IntPtr pMessage) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      SWIGPendingException.Set(new global::System.SystemException(message, SWIGPendingException.Retrieve()));
    }

    [$imclassname.MonoPInvokeCallback(typeof(ExceptionArgumentDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingArgumentException(System.IntPtr pMessage, System.IntPtr pParamName) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      string paramName = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pParamName);
      SWIGPendingException.Set(new global::System.ArgumentException(message, paramName, SWIGPendingException.Retrieve()));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionArgumentDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
    static void SetPendingArgumentNullException(System.IntPtr pMessage, System.IntPtr pParamName) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      string paramName = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pParamName);
      global::System.Exception e = SWIGPendingException.Retrieve();
      if (e != null) message = message + " Inner Exception: " + e.Message;
      SWIGPendingException.Set(new global::System.ArgumentNullException(paramName, message));
    }
    [$imclassname.MonoPInvokeCallback(typeof(ExceptionArgumentDelegate))]
#if NET6_0_OR_GREATER
    [global::System.Runtime.InteropServices.UnmanagedCallersOnly(CallConvs = new[] { typeof(System.Runtime.CompilerServices.CallConvCdecl) })]
#endif
      static void SetPendingArgumentOutOfRangeException(System.IntPtr pMessage, System.IntPtr pParamName) {
      string message = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pMessage);
      string paramName = global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(pParamName);
      global::System.Exception e = SWIGPendingException.Retrieve();
      if (e != null) message = message + " Inner Exception: " + e.Message;
      SWIGPendingException.Set(new global::System.ArgumentOutOfRangeException(paramName, message));
    }

    static SWIGExceptionHelper() {
      SWIGRegisterExceptionCallbacks_$module(
                                applicationDelegate,
                                arithmeticDelegate,
                                divideByZeroDelegate,
                                indexOutOfRangeDelegate,
                                invalidCastDelegate,
                                invalidOperationDelegate,
                                ioDelegate,
                                nullReferenceDelegate,
                                outOfMemoryDelegate,
                                overflowDelegate,
                                systemDelegate);

      SWIGRegisterExceptionCallbacksArgument_$module(
                                argumentDelegate,
                                argumentNullDelegate,
                                argumentOutOfRangeDelegate);
    }
  }

  protected static SWIGExceptionHelper swigExceptionHelper = new SWIGExceptionHelper();

  public class SWIGPendingException {
    [global::System.ThreadStatic]
    private static global::System.Exception pendingException = null;
    private static int numExceptionsPending = 0;
    private static global::System.Object exceptionsLock = null;

    public static bool Pending {
      get {
        bool pending = false;
        if (numExceptionsPending > 0)
          if (pendingException != null)
            pending = true;
        return pending;
      } 
    }

    public static void Set(global::System.Exception e) {
      if (pendingException != null)
        throw new global::System.ApplicationException("FATAL: An earlier pending exception from unmanaged code was missed and thus not thrown (" + pendingException.ToString() + ")", e);
      pendingException = e;
      lock(exceptionsLock) {
        numExceptionsPending++;
      }
    }

    public static global::System.Exception Retrieve() {
      global::System.Exception e = null;
      if (numExceptionsPending > 0) {
        if (pendingException != null) {
          e = pendingException;
          pendingException = null;
          lock(exceptionsLock) {
            numExceptionsPending--;
          }
        }
      }
      return e;
    }

    static SWIGPendingException() {
      exceptionsLock = new global::System.Object();
    }
  }
%}
#endif // SWIG_CSHARP_NO_EXCEPTION_HELPER

#if !defined(SWIG_CSHARP_NO_STRING_HELPER)
%insert(runtime) %{
/* Callback for returning strings to C# without leaking memory */
typedef char * (SWIGSTDCALL* SWIG_CSharpStringHelperCallback)(const char *);
static SWIG_CSharpStringHelperCallback SWIG_csharp_string_callback = NULL;
%}

%pragma(csharp) imclasscode=%{
  protected class SWIGStringHelper {

#if __IOS__
    [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
    public delegate string SWIGStringDelegate(System.IntPtr message);
    static SWIGStringDelegate stringDelegate = new SWIGStringDelegate(CreateString);

    [global::System.Runtime.InteropServices.DllImport($dllimport, EntryPoint="SWIGRegisterStringCallback_$module")]
    public static extern void SWIGRegisterStringCallback_$module(SWIGStringDelegate stringDelegate);

    [$imclassname.MonoPInvokeCallback(typeof(SWIGStringDelegate))]
    static string CreateString(System.IntPtr cString) {
      return global::System.Runtime.InteropServices.Marshal.PtrToStringAnsi(cString);
    }

    static SWIGStringHelper() {
      SWIGRegisterStringCallback_$module(stringDelegate);
    }
  }

  static protected SWIGStringHelper swigStringHelper = new SWIGStringHelper();
%}

%insert(runtime) %{
#ifdef __cplusplus
extern "C" 
#endif
SWIGEXPORT void SWIGSTDCALL SWIGRegisterStringCallback_$module(SWIG_CSharpStringHelperCallback callback) {
  SWIG_csharp_string_callback = callback;
}
%}
#endif // SWIG_CSHARP_NO_STRING_HELPER

#if !defined(SWIG_CSHARP_NO_IMCLASS_STATIC_CONSTRUCTOR)
// Ensure the class is not marked beforefieldinit
%pragma(csharp) imclasscode=%{
  static $imclassname() {
  }
%}
#endif

%insert(runtime) %{
/* Contract support */

#define SWIG_contract_assert(nullreturn, expr, msg) if (!(expr)) {SWIG_CSharpSetPendingExceptionArgument(SWIG_CSharpArgumentOutOfRangeException, msg, ""); return nullreturn; } else
%}

// rbfx
%pragma(csharp) imclasscode=%{
    internal class OverrideNativeAttribute : global::System.Attribute
    {
    }

    internal static bool MethodHasAttribute<T>(global::System.Reflection.MethodInfo method) where T : global::System.Attribute
    {
        var attributes = method.GetCustomAttributes(typeof(T), true);
        return attributes.Length > 0;
    }

    internal static bool SwigDerivedClassHasMethod(object instance, string methodName, global::System.Type classType, global::System.Type[] methodTypes) {
      global::System.Reflection.MethodInfo methodInfo = instance.GetType().GetMethod(
          methodName, global::System.Reflection.BindingFlags.Public |
                      global::System.Reflection.BindingFlags.NonPublic |
                      global::System.Reflection.BindingFlags.Instance, null, methodTypes, null);
      bool hasDerivedMethod = methodInfo.IsVirtual && !methodInfo.IsAbstract &&
                              (methodInfo.DeclaringType.IsSubclassOf(classType) || methodInfo.DeclaringType == classType) &&
                              (methodInfo.DeclaringType != methodInfo.GetBaseDefinition().DeclaringType || MethodHasAttribute<OverrideNativeAttribute>(methodInfo));
      /* Could add this code to cover corner case where the GetMethod() returns a method which allows type
       * promotion, eg it will return foo(double), if looking for foo(int).
       if (hasDerivedMethod) {
         hasDerivedMethod = false;
         if (methodInfo != null) {
           hasDerivedMethod = true;
           ParameterInfo[] parameterArray1 = methodInfo.GetParameters();
           for (int i=0; i<methodTypes.Length; i++) {
             if (parameterArray1[0].ParameterType != methodTypes[0]) {
               hasDerivedMethod = false;
               break;
             }
           }
         }
       }
       */
      return hasDerivedMethod;
    }
%}
// /rbfx
