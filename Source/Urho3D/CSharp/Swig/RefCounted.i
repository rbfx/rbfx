// This could be optimized by modifying shared_ptr<> typemaps to store a raw pointer instead of new shared_ptr instance
// and increment/decrement refcount manually.
%define URHO3D_REFCOUNTED(TYPE)
    %typemap(csbody) TYPE %{
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      private bool _disposing = true;
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        // This function gets called when we want to turn a native cPtr into managed instance. For example when some
        // method returns a pointer to a wrapped object.
        if (cPtr == global::System.IntPtr.Zero)
          return null;
        // Obtain GCHandle stored in RefCounted.
        var handle = $imclassname.RefCounted_GetScriptObject(new global::System.Runtime.InteropServices.HandleRef(null, cPtr));
        $csclassname result = null;
        if (handle != global::System.IntPtr.Zero) {
          // Fetch managed object from returned GCHandle and return it as this particular cPtr already has a wrapper object instance.
          var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
          return ($csclassname)gchandle.Target;
        }
        // GCHandle is null. cPtr does not have a managed wrapper object yet. We will create one.
        global::System.Type type;
        // Look up a most downstream type in type registry. This is important, because it allows us to use correct
        // wrapper type for polymorphic classes. For example API may return "UIElement" type, but cPtr would actually be
        // of type "Text". Here we would fetch type "Text" from type registry.
        if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
          type = typeof($csclassname);
        if (type == typeof($csclassname))
          // A fast path when type is not polymorphic. Using activator is not exactly cheap.
          result = new $csclassname(cPtr, cMemoryOwn);
        else
          // Type is polymorphic. Use activator to construct object from opaque Type object.
          result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
        return result;
      }

      internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) {
        // Construct a wrapper object wrapping a native instance.
        // Only most downstream class gets cMemoryOwn=true. Base classes do not.
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
        global::System.Runtime.InteropServices.GCHandleType handleType;
        if (cMemoryOwn)
          AddRef();
        SetScriptObject(global::System.Runtime.InteropServices.GCHandle.ToIntPtr(
          global::System.Runtime.InteropServices.GCHandle.Alloc(this, global::System.Runtime.InteropServices.GCHandleType.Weak)), false);
      }

      internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }
    %}

    %typemap(csbody_derived, directorsetup="\n        SetupSwigDirector();") TYPE %{
      [global::System.Diagnostics.DebuggerBrowsable(global::System.Diagnostics.DebuggerBrowsableState.Never)]
      private global::System.Runtime.InteropServices.HandleRef swigCPtr;
      internal static $csclassname wrap(global::System.IntPtr cPtr, bool cMemoryOwn) {
        // This function gets called when we want to turn a native cPtr into managed instance. For example when some
        // method returns a pointer to a wrapped object.
        if (cPtr == global::System.IntPtr.Zero)
          return null;
        // Obtain GCHandle stored in RefCounted.
        var handle = $imclassname.RefCounted_GetScriptObject(new global::System.Runtime.InteropServices.HandleRef(null, cPtr));
        $csclassname result = null;
        if (handle != global::System.IntPtr.Zero) {
          // Fetch managed object from returned GCHandle and return it as this particular cPtr already has a wrapper object instance.
          var gchandle = global::System.Runtime.InteropServices.GCHandle.FromIntPtr(handle);
          return ($csclassname)gchandle.Target;
        }
        // GCHandle is null. cPtr does not have a managed wrapper object yet. We will create one.
        global::System.Type type;
        // Look up a most downstream type in type registry. This is important, because it allows us to use correct
        // wrapper type for polymorphic classes. For example API may return "UIElement" type, but cPtr would actually be
        // of type "Text". Here we would fetch type "Text" from type registry.
        if (!$imclassname.SWIGTypeRegistry.TryGetValue($imclassname.$csclazznameSWIGTypeId(cPtr), out type))
          type = typeof($csclassname);
        if (type == typeof($csclassname))
          // A fast path when type is not polymorphic. Using activator is not exactly cheap.
          result = new $csclassname(cPtr, cMemoryOwn);
        else
          // Type is polymorphic. Use activator to construct object from opaque Type object.
          result = ($csclassname)global::System.Activator.CreateInstance(type, global::System.Reflection.BindingFlags.Instance|global::System.Reflection.BindingFlags.NonPublic|global::System.Reflection.BindingFlags.Public, null, new object[]{cPtr, cMemoryOwn}, null);
        return result;
      }

      internal $csclassname(global::System.IntPtr cPtr, bool cMemoryOwn) : base($imclassname.$csclazznameSWIGUpcast(cPtr), false) {
        // Construct a wrapper object wrapping a native instance.
        // Only most downstream class gets cMemoryOwn=true. Base classes do not.
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);$directorsetup
        global::System.Runtime.InteropServices.GCHandleType handleType;
        if (cMemoryOwn)
          AddRef();
      }

      internal static global::System.Runtime.InteropServices.HandleRef getCPtr($csclassname obj) {
        return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
      }
    %}

    %typemap(csdispose) TYPE %{
      ~$csclassname() {
        _disposing = false;
        if (!Expired) {
          // Finalizer should only execute if only .NET held references to a managed object and lost them.
          global::System.Diagnostics.Debug.Assert(ScriptRefs() == Refs());
          // Release all references but one here on this thread.
          while (ScriptRefs() > 1)
            ReleaseRef();
          // Here be dragons. This evil hack prevents crashes on application exit. Lost objects are being finalized on
          // finalizer thread, however runtime scripting API object is gone already and we can not invoke Dispose(false)
          // from C++ side, therefore we do it manually.
          var ptrBkp = swigCPtr;                                        // Native instance pointer backup.
          Dispose(_disposing);                                          // Dispose of resources. This resets swigCPtr. Also frees GC handle.
          swigCPtr = ptrBkp;                                            // Restore swigCPtr so we can call ReleaseRef().
          if (ScriptRefs() == 1) {                                      // Last remaining reference.
            var script = Context.Instance?.GetSubsystem<Script>();      // Wont return an object when finalizers run on application exit.
            if (script != null)
              script.ReleaseRefOnMainThread(this);                      // Called when objects are lost and finalized during application lifetime.
            else
              ReleaseRef();                                             // Called when application is quitting. Main loop is not running.
          }
          // Object is completely destroyed. Reset swigCPtr so Expired once again returns true.
          swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
        }
      }

      public void Dispose() {
        // Dispose() only releases a reference. Releasing last reference will call obj.Dispose(true). This is to allow
        // use of objects with C# `using` statement. Finalization suppression is in RefCounted.DisposeInternal().
        ReleaseRef();
      }
    %}
    %typemap(csdispose_derived) TYPE ""

    %typemap(csdisposing, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
      lock(this) {
        $typemap(csdisposed_extra_early_optional, TYPE)
        FreeGCHandle();
        if (swigCPtr.Handle != global::System.IntPtr.Zero)
          swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
        if (disposing) {
          global::System.GC.SuppressFinalize(this);
        }
        $typemap(csdisposed_extra_optional, TYPE)
      }
    }

    %typemap(csdisposing_derived, methodname="Dispose", methodmodifiers="protected", parameters="bool disposing") TYPE {
      lock(this) {
        $typemap(csdisposed_extra_early_optional, TYPE)
        if (swigCPtr.Handle != global::System.IntPtr.Zero)
          swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
        base.Dispose(disposing);
        $typemap(csdisposed_extra_optional, TYPE)
      }
    }
    // C# wrapper does this instead
    %refobject   TYPE "//$this->AddRef();        // Called by C# wrapper wrap() or constructor."
    %unrefobject TYPE "//$this->ReleaseRef();    // Called by C# wrapper Dispose().\n  assert(false);   // This destructor should never be called, ReleaseRef() handles destruction."

    // SharedPtr
    %typemap(ctype)  Urho3D::SharedPtr<TYPE> "TYPE*"                               // c layer type
    %typemap(imtype) Urho3D::SharedPtr<TYPE> "global::System.IntPtr"               // pinvoke type
    %typemap(cstype) Urho3D::SharedPtr<TYPE> "$typemap(cstype, TYPE*)"             // c# type
    %typemap(in)     Urho3D::SharedPtr<TYPE> %{ $1 = Urho3D::SharedPtr<TYPE>($input); %}    // c to cpp
    %typemap(out)    Urho3D::SharedPtr<TYPE> %{ $result = $1.Detach(); %}          // cpp to c
    %typemap(out)    TYPE*          %{ $result = $1;                   %}          // cpp to c
    %typemap(csin)   Urho3D::SharedPtr<TYPE> "$typemap(cstype, TYPE*).getCPtr($csinput).Handle"      // convert C# to pinvoke
    %typemap(csout, excode=SWIGEXCODE) Urho3D::SharedPtr<TYPE> {                   // convert pinvoke to C#
        var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }
    %typemap(directorin)  Urho3D::SharedPtr<TYPE> "$iminput"
    %typemap(directorout) Urho3D::SharedPtr<TYPE> "$cscall"

    %typemap(csvarin, excode=SWIGEXCODE2) Urho3D::SharedPtr<TYPE> & %{
    set {
        $imcall;$excode
    }
    %}
    %typemap(csvarout, excode=SWIGEXCODE2) Urho3D::SharedPtr<TYPE> & %{
    get {
        var ret = $typemap(cstype, TYPE*).wrap($imcall, true);$excode
        return ret;
    }
    %}

    %apply Urho3D::SharedPtr<TYPE> { Urho3D::SharedPtr<TYPE>& }

    %typemap(in) Urho3D::SharedPtr<TYPE> & %{
        $*1_ltype $1Ref($input);
        $1 = &$1Ref;
    %}
    %typemap(out) Urho3D::SharedPtr<TYPE> & %{ $result = $1->Get(); %}             // cpp to c

    // WeakPtr
    %apply Urho3D::SharedPtr<TYPE>  { Urho3D::WeakPtr<TYPE>  }
    %apply Urho3D::SharedPtr<TYPE>& { Urho3D::WeakPtr<TYPE>& }
    %typemap(out) Urho3D::WeakPtr<TYPE> %{ $result = $1.Get();          %}         // cpp to c
    %typemap(out) Urho3D::WeakPtr<TYPE> & %{ $result = $1->Get(); %}               // cpp to c
    %typemap(in) Urho3D::WeakPtr<TYPE> & %{
        $*1_ltype $1Ref($input);
        $1 = &$1Ref;
    %}
%enddef

// TODO: Fix autoswig script to output these as well.
URHO3D_REFCOUNTED(Urho3D::RefCounted);
URHO3D_REFCOUNTED(Urho3D::ShaderProgram);

%include "_refcounted.i"
//%interface_custom("%s", "I%s", Urho3D::RefCounted);
%director Urho3D::RefCounted;
%ignore Urho3D::RefCounted::RefCountPtr;
%ignore Urho3D::RefCount;
%csmethodmodifiers Urho3D::RefCounted::SetScriptObject "internal"
%csmethodmodifiers Urho3D::RefCounted::GetScriptObject "internal"
%csmethodmodifiers Urho3D::RefCounted::AddRef "internal"
%csmethodmodifiers Urho3D::RefCounted::ReleaseRef "internal"
%rename(NativeAddRef) Urho3D::RefCounted::AddRef;
%rename(NativeReleaseRef) Urho3D::RefCounted::ReleaseRef;
%rename(AddRef) Urho3D::RefCounted::ScriptAddRef;
%rename(ReleaseRef) Urho3D::RefCounted::ScriptReleaseRef;
%include "Urho3D/Container/RefCounted.h"
%include "Urho3D/Container/Ptr.h"
